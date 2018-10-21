//
// storage-engine.cpp
//
//  Created by Peter Gusev on 27 May 2018.
//  Copyright 2013-2018 Regents of the University of California
//

#include "storage-engine.hpp"

#include <unordered_map>
#include <ndn-cpp/data.hpp>
#include <ndn-cpp/interest.hpp>
#include <ndn-cpp/name.hpp>
#include <ndn-cpp/util/blob.hpp>
#include <boost/algorithm/string.hpp>

#include "../config.hpp"

#if HAVE_LIBROCKSDB

#ifndef __ANDROID__ // use RocksDB on linux and macOS
    
    #include <rocksdb/db.h>
    namespace db_namespace = rocksdb;

#else // for Android - use LevelDB

    #include <leveldb/db.h>
    namespace db_namespace = leveldb;

#endif

#endif

using namespace fast_repo;
using namespace ndn;
using namespace boost;

//******************************************************************************
namespace fast_repo {

class StorageEngineImpl : public enable_shared_from_this<StorageEngineImpl> {
    public:
        typedef struct _Stats {
            size_t nKeys_;
            size_t valueSizeBytes_;
        } Stats;

    #if HAVE_LIBROCKSDB
        StorageEngineImpl(std::string dbPath): dbPath_(dbPath), db_(nullptr), keysTrieBuilt_(false)
        {
        }
    #else
        StorageEngineImpl(std::string dbPath) {
            throw std::runtime_error("The library is not copmiled with persistent storage support.");
        }
    #endif

        ~StorageEngineImpl()
        {
            close();
        }

        bool open(bool readOnly)
        {
#if HAVE_LIBROCKSDB
            db_namespace::Options options;
            options.create_if_missing = true;
            db_namespace::Status status;
            if (readOnly)
                status = db_namespace::DB::OpenForReadOnly(options, dbPath_, &db_);
            else
                status = db_namespace::DB::Open(options, dbPath_, &db_);
            
            if (!status.ok())
                throw std::runtime_error(status.getState());

            return status.ok();
#else
            return false;
#endif
        }

        void close()
        {
#if HAVE_LIBROCKSDB
            if (db_)
                delete db_;
#endif
        }

        bool put(const Data& data)
        {
#if HAVE_LIBROCKSDB
            if (!db_)
                throw std::runtime_error("DB is not open");

            db_namespace::Status s = 
                db_->Put(db_namespace::WriteOptions(),
                         data.getName().toUri(),
                         db_namespace::Slice((const char*)data.wireEncode().buf(),
                                             data.wireEncode().size()));
            return s.ok();
#else
            return false;
#endif
        }

        shared_ptr<Data> get(const Name& dataName)
        {
#if HAVE_LIBROCKSDB
            if (!db_)
                throw std::runtime_error("DB is not open");

            static std::string dataString;
            db_namespace::Status s = db_->Get(db_namespace::ReadOptions(),
                                              dataName.toUri(),
                                              &dataString);
            if (s.ok())
            {
                shared_ptr<Data> data = make_shared<Data>();
                data->wireDecode((const uint8_t*)dataString.data(), dataString.size());
                
                return data;
            }
#endif
            return shared_ptr<Data>(nullptr);
        }

        shared_ptr<Data> read(const Interest& interest)
        {
            // TODO: implement prefix match data retrieval 
            return get(interest.getName());
        }

        void getLongestPrefixes(asio::io_service& io, 
            function<void(const std::vector<Name>&)> onCompletion)
        {
            // if (!keysTrieBuilt_)
            //     onCompletion(keysTrie_.getLongestPrefixes());
            // else
            // {
                shared_ptr<StorageEngineImpl> me = shared_from_this();
                io.dispatch([me,this,onCompletion](){
                    buildKeyTrie();
                    keysTrieBuilt_ = true;
                    onCompletion(keysTrie_.getLongestPrefixes());
                });
            // }
        }

        const Stats& getStats() const { return stats_; }

    private:
        class NameTrie {
        public: 
            struct TrieNode {
                bool isLeaf;
                std::unordered_map<std::string, shared_ptr<TrieNode>> components;

                TrieNode():isLeaf(false){}
            };

            NameTrie():head_(make_shared<TrieNode>()){}

            void insert(const std::string& n) {

                shared_ptr<TrieNode> curr = head_;
                std::vector<std::string> components;
                split(components, n, boost::is_any_of("/"));

                for (auto c:components)
                {
                    if (c.size() == 0)
                        continue;
                    if (curr->components.find(c) == curr->components.end())
                        curr->components[c] = make_shared<TrieNode>();
                    curr = curr->components[c];
                }
            }

            // gets all longest prefixes
            const std::vector<Name> getLongestPrefixes() const {
                std::vector<Name> longestPrefixes;

                for (auto cIt:head_->components)
                {
                    Name n(cIt.first);
                    shared_ptr<TrieNode> curr = cIt.second;

                    while (curr.get() && !curr->isLeaf && curr->components.size() == 1){
                        auto it = curr->components.begin();
                        n.append(Name::fromEscapedString(it->first));
                        curr = it->second;
                    }
                    longestPrefixes.push_back(n);
                }

                return longestPrefixes;
            }
        private:
            shared_ptr<TrieNode> head_;
        };

        std::string dbPath_;
        bool keysTrieBuilt_;
        NameTrie keysTrie_;
        Stats stats_;
#if HAVE_LIBROCKSDB
        db_namespace::DB* db_;
#endif

        void buildKeyTrie()
        {
            stats_.nKeys_ = 0;
            stats_.valueSizeBytes_ = 0;
#if HAVE_LIBROCKSDB

            db_namespace::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());

            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                keysTrie_.insert(it->key().ToString());
                stats_.nKeys_++;
                stats_.valueSizeBytes_ += it->value().size();
            }
            assert(it->status().ok()); // Check for any errors found during the scan

            delete it;
#endif
        }
};

}

//******************************************************************************
StorageEngine::StorageEngine(std::string dbPath, bool readOnly):
    pimpl_(boost::make_shared<StorageEngineImpl>(dbPath))
{
    try {
        pimpl_->open(readOnly);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error("Failed to open storage at "+dbPath+": "+e.what());
    }
}

StorageEngine::~StorageEngine()
{
    pimpl_->close();
}

void StorageEngine::put(const shared_ptr<const Data>& data)
{
    pimpl_->put(*data);
}

void StorageEngine::put(const Data& data)
{
    pimpl_->put(data);
}

shared_ptr<Data> 
StorageEngine::get(const Name& dataName)
{
    return pimpl_->get(dataName);
}

shared_ptr<Data>
StorageEngine::read(const Interest& interest)
{
    return pimpl_->read(interest);
}

void 
StorageEngine::scanForLongestPrefixes(asio::io_service& io, 
            function<void(const std::vector<ndn::Name>&)> onCompleted)
{
    pimpl_->getLongestPrefixes(io, onCompleted);
}

const size_t
StorageEngine::getPayloadSize() const
{
    return pimpl_->getStats().valueSizeBytes_;
}

const size_t
StorageEngine::getKeysNum() const
{
    return pimpl_->getStats().nKeys_;
}