//
// fast-repo.cpp
//
//  Created by Peter Gusev on 16 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "fast-repo.hpp"

#include <fstream>
#include <boost/property_tree/info_parser.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ndn-cpp/name.hpp>
#include <ndn-cpp/face.hpp>

#include "../storage/storage-engine.hpp"

using namespace fast_repo;
using namespace ndn;

using std::shared_ptr;
using std::make_shared;
using std::enable_shared_from_this;

static Config DefaultConfig = Config();

namespace fast_repo
{
class FastRepoImpl : public enable_shared_from_this<FastRepoImpl>
{
  public:
    FastRepoImpl(boost::asio::io_service &io,
                 const Config &config,
                 const shared_ptr<ndn::Face> &face,
                 const shared_ptr<ndn::KeyChain> &keyChain);
    ~FastRepoImpl();

    void enableListening();
    void enableValidation();
    void initializeStorage();

  private:
    boost::asio::io_service &io_;
    Config config_;
    shared_ptr<ndn::Face> face_;
    shared_ptr<ndn::KeyChain> keyChain_;
    shared_ptr<StorageEngine> storageEngine_;

    repo_ng::ReadHandle readHandle_;
#if 0
    repo::WriteHandle writeHandle_;
    repo::WatchHandle watchHandle_;
    repo::DeleteHandle deleteHandle_;
#endif
    PatternHandle patternHandle_;
    repo_ng::WriteHandle writeHandle_;

    //ndn::Validator validator_;

    void onDataInsertion(const Name& prefix);
};
} // namespace fast_repo

Config
fast_repo::parseConfig(const std::string &configPath)
{
    if (configPath.empty())
    {
        std::cerr << "configuration file path is empty" << std::endl;
    }

    std::ifstream fin(configPath.c_str());
    if (!fin.is_open())
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to open configuration file '" + configPath + "'"));

    using namespace boost::property_tree;
    ptree propertyTree;
    try
    {
        read_info(fin, propertyTree);
    }
    catch (const ptree_error &e)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to read configuration file '" + configPath + "'"));
    }

    ptree repoConf = propertyTree.get_child("repo");

    Config repoConfig;
    repoConfig.repoConfigPath = configPath;

    ptree dataConf = repoConf.get_child("data");
    for (const auto &section : dataConf)
    {
        if (section.first == "prefix")
            repoConfig.dataPrefixes.push_back(Name(section.second.get_value<std::string>()));
        else if (section.first == "registration-subset")
            repoConfig.registrationSubset = section.second.get_value<int>();
        else
            BOOST_THROW_EXCEPTION(std::runtime_error("Unrecognized '" + section.first + "' option in 'data' section in "
                                                                                        "configuration file '" +
                                                     configPath + "'"));
    }

    ptree commandConf = repoConf.get_child("command");
    for (const auto &section : commandConf)
    {
        if (section.first == "prefix")
            repoConfig.repoPrefixes.push_back(Name(section.second.get_value<std::string>()));
        else
            BOOST_THROW_EXCEPTION(std::runtime_error("Unrecognized '" + section.first + "' option in 'command' section in "
                                                                                        "configuration file '" +
                                                     configPath + "'"));
    }

    if (repoConf.get<std::string>("storage.method") != "rocksdb")
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Only 'rocksdb' storage method is supported"));
    }

    repoConfig.readOnly = (repoConf.get<std::string>("storage.mode") == "read_only");
    repoConfig.dbPath = repoConf.get<std::string>("storage.path");
    repoConfig.validatorNode = repoConf.get_child("validator");

    return repoConfig;
}

//***
FastRepo::FastRepo(boost::asio::io_service &io,
                   const Config &config,
                   const shared_ptr<ndn::Face> &face,
                   const shared_ptr<ndn::KeyChain> &keyChain)
    : pimpl_(make_shared<FastRepoImpl>(io, config, face, keyChain))
{
    pimpl_->initializeStorage();
}

void FastRepo::enableListening()
{
    pimpl_->enableListening();
}

void FastRepo::enableValidation()
{
    pimpl_->enableValidation();
}

//***
FastRepoImpl::FastRepoImpl(boost::asio::io_service &io,
                           const Config &config,
                           const shared_ptr<ndn::Face> &face,
                           const shared_ptr<ndn::KeyChain> &keyChain)
    : io_(io), config_(config), face_(face), keyChain_(keyChain), storageEngine_(make_shared<StorageEngine>(config_.dbPath, config_.readOnly))
    , readHandle_(*face_, *storageEngine_, *keyChain_), patternHandle_(*face_, *storageEngine_, *keyChain_), writeHandle_(*face_, *storageEngine_, *keyChain_)

{
}

FastRepoImpl::~FastRepoImpl()
{
    // TODO: unregister prefixes
}

void FastRepoImpl::enableListening()
{
    for (const ndn::Name &cmdPrefix : config_.repoPrefixes)
    {
        face_->registerPrefix(cmdPrefix,
                              [](const shared_ptr<const Name> &prefix,
                                 const shared_ptr<const Interest> &interest,
                                 Face &face, uint64_t, const shared_ptr<const InterestFilter> &) {
                                     std::cerr << "unexpected interest received: " << interest->getName() << std::endl;
                              },
                              [](const shared_ptr<const Name> &cmdPrefix) {
                                  std::cerr << "failed to register prefix " << cmdPrefix << std::endl;
                                  BOOST_THROW_EXCEPTION(std::runtime_error("Command prefix registration failed"));
                              },
                              [](const shared_ptr<const Name>& prefix,
                                 uint64_t registeredPrefixId){
                                     std::cout << "registered cmd prefix " << *prefix << std::endl;
                                 });

        writeHandle_.listen(cmdPrefix);
        // m_watchHandle.listen(cmdPrefix);
        // m_deleteHandle.listen(cmdPrefix);
        patternHandle_.listen(cmdPrefix);

        writeHandle_.onDataInsertion.connect(bind(&FastRepoImpl::onDataInsertion, this, _1));
        patternHandle_.onDataInsertion.connect(bind(&FastRepoImpl::onDataInsertion, this, _1));
    }
}

void FastRepoImpl::enableValidation()
{
}

void FastRepoImpl::initializeStorage()
{
    std::cout << "opened storage in " << (config_.readOnly ? "readonly" : "read-write")
              << " mode at " << config_.dbPath << std::endl;

    shared_ptr<FastRepoImpl> me = shared_from_this();
    storageEngine_->scanForLongestPrefixes(io_, [me, this](const std::vector<ndn::Name> &prefixes) {
        for (auto p : prefixes)
            config_.dataPrefixes.push_back(Name(p));
        for (auto p : config_.dataPrefixes)
        {
            readHandle_.listen(p);
            std::cout << "registered data prefix: " << p << std::endl;
        }
    });
}

void FastRepoImpl::onDataInsertion(const Name& prefix)
{
    // Ensure no such prefix is listening
    for (const auto& p : config_.dataPrefixes){
        if(p.isPrefixOf(prefix))
        {
            return ;
        }
    }

    // Let ReadHandle to listen to it
    config_.dataPrefixes.push_back(Name(prefix));
    readHandle_.listen(prefix);
    std::cout << "registered data prefix: " << prefix << std::endl;
}
