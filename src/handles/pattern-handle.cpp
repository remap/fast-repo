//
// pattern-handle.cpp
//
//  Created by Peter Gusev on 17 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "pattern-handle.hpp"

#include <ndn-cpp/face.hpp>
#include <ndn-cpp/interest-filter.hpp>

using namespace fast_repo;
using namespace ndn;

/**
 * A simple fetch pattern, for test.
 * I don't know whether a "fetch pattern" looks like this or not.
 */
class CounterPattern : public IFetchPattern{
private:
    bool running_;
    int counter_;
    StoreData storeFun_;
    ndn::Face *face_;
    ndn::Name fetchPrefix_;

public:
    CounterPattern():running_(false)
    { }

    const ndn::Name::Component getPatternKeyword() const override
    {
        return "counter";
    }

    void cancel() override
    {
        // TODO: Improve implementation (in a real pattern)
        running_ = false;
    }

    // I think there will be little chance to give a different face, keyChain or storePacketFun
    // Maybe we can change the interface?
    void fetch(ndn::Face & face, ndn::KeyChain & keyChain,
               const ndn::Name &prefix, StoreData storePacketFun) override
    {
        fetchPrefix_ = prefix;
        storeFun_ = storePacketFun;
        face_ = &face;
        //keyChain_ = &keyChain;
        counter_ = 0;

        face_->callLater(1.0, ndn::func_lib::bind(&CounterPattern::doFetch, this));

        running_ = true;
    }

    void doFetch(){
        if(!running_)
            return;
        ndn::Interest fetchInterest(ndn::Name(fetchPrefix_).append(std::to_string(counter_)));
        std::cout << "Counter interest: " << fetchInterest.getName() << std::endl; //////TEST
        fetchInterest.setInterestLifetimeMilliseconds(4000.0);
        face_->expressInterest(fetchInterest, 
                               bind(&CounterPattern::onData, this, ndn::func_lib::_1, ndn::func_lib::_2));
    }

    void onData(const std::shared_ptr<const ndn::Interest>& interest,
                const std::shared_ptr<ndn::Data>& data)
    {
        // Put data without validation
        std::cout << "On counter data: " << data->getName() << std::endl; //////TEST
        storeFun_(*data);

        // Schedule the next fetch
        counter_ ++;
        if(counter_ < 10){
            face_->callLater(2000.0, ndn::func_lib::bind(&CounterPattern::doFetch, this));
        }else{
            running_ = false;
        }
    }
};

PatternHandle::PatternHandle(ndn::Face &face, StorageEngine &storage, ndn::KeyChain &keyChain)
    : BaseHandle(face, storage, keyChain)
{
    addPattern(ndn::ptr_lib::make_shared<CounterPattern>());
}

void PatternHandle::listen(const ndn::Name &prefix)
{
    std::cout << "PatternHandle Listen: " << ndn::Name(prefix).append("pattern") << std::endl; //////TEST
    getFace().setInterestFilter(ndn::Name(prefix).append("pattern"),
                                std::bind(&PatternHandle::onInterest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
}

void PatternHandle::addPattern(std::shared_ptr<IFetchPattern> p)
{
    patterns_[p->getPatternKeyword()] = p;
}

void PatternHandle::removePattern(std::shared_ptr<IFetchPattern> p)
{
    if (patterns_.find(p->getPatternKeyword()) != patterns_.end())
        patterns_.erase(p->getPatternKeyword());
}

bool PatternHandle::decodeNames(const ndn_message::RepoCommandParameterMessage_Name &composed,
                                ndn::Name &patternName,
                                ndn::Name &fetchPrefix)
{
    if(composed.component_size() != 2){
        return false;
    }

    try{
        auto decodeString = [&](const std::string& str, ndn::Name& result)->void{
            ndn_message::RepoCommandParameterMessage_Name tmpName;
            ndn::ProtobufTlv::decode(tmpName, (uint8_t*)str.c_str(), str.size());
            result.clear();
            for(size_t i = 0; i < tmpName.component_size(); i ++){
                result.append(tmpName.component(i));
            }
        };
        decodeString(composed.component(0), patternName);
        decodeString(composed.component(1), fetchPrefix);
    }catch (std::exception& e){
        return false;
    }
    return true;
}

void PatternHandle::onInterest(const std::shared_ptr<const ndn::Name> &prefix,
                               const std::shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
                               uint64_t interestFilterId,
                               const std::shared_ptr<const ndn::InterestFilter> &filter)
{
    std::cout << "On pattern: " << interest->getName() << std::endl; //////TEST

    ndn_message::RepoCommandParameterMessage parameter;
    try{
        extractParameter(*interest, *prefix, parameter);
    }
    catch (std::exception& e){
        negativeReply(*interest, 403);
        return;
    }

    // ABUSE Name field: compo[0]:=PatternName(only 1st component); compo[1]:=FetchPrefix;
    Name patternName, fetchPrefix;
    if(!decodeNames(parameter.repo_command_parameter().name(), patternName, fetchPrefix)){
        negativeReply(*interest, 403);
        return;
    }

    std::shared_ptr<IFetchPattern> p;
    auto patIt = patterns_.find(patternName.get(0));
    // If p is not found, 
    // Result code: 404:=No such pattern is known
    if(patIt == patterns_.end()){
        negativeReply(*interest, 404);
        return;
    }
    p = patIt->second;

    if (p)
    {
        p->fetch(getFace(), getKeyChain(), fetchPrefix,
                 std::bind(static_cast<void(StorageEngine::*)(const ndn::Data&)>(&StorageEngine::put), 
                      &getStorageHandle(), std::placeholders::_1));
    }else{
        // This should be impossible
        throw Error("Pattern pool is broken.");
    }
}