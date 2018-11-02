//
// pattern-handle.cpp
//
//  Created by Peter Gusev on 17 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "pattern-handle.hpp"

#include "patterns/counter-pattern.hpp"

#include <ndn-cpp/face.hpp>
#include <ndn-cpp/interest-filter.hpp>

using namespace fast_repo;
using namespace ndn;

using std::bind;
using std::shared_ptr;
using std::make_shared;

PatternHandle::PatternHandle(ndn::Face &face, StorageEngine &storage, ndn::KeyChain &keyChain)
    : BaseHandle(face, storage, keyChain)
    , patternFactory_(PatternFactory::getInstance())
{
}

void PatternHandle::listen(const ndn::Name &prefix)
{
    getFace().setInterestFilter(ndn::Name(prefix).append("pattern"),
                                bind(&PatternHandle::onInterest, this, _1, _2, _3, _4, _5));
    getFace().setInterestFilter(ndn::Name(prefix).append("cancel"),
                                bind(&PatternHandle::onCancelRequest, this, _1, _2, _3, _4, _5));
}

void PatternHandle::removePattern(const ndn::Name &fetchPrefix)
{
    auto patIt = patterns_.find(fetchPrefix);
    if(patIt != patterns_.end())
    {
        patIt->second->cancel();
        patterns_.erase(patIt);
    }
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

void PatternHandle::onInterest(const shared_ptr<const ndn::Name> &prefix,
                               const shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
                               uint64_t interestFilterId,
                               const shared_ptr<const ndn::InterestFilter> &filter)
{
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

    auto patIt = patterns_.find(fetchPrefix);
    // If p is found, 
    // Result code: 402:=Duplicated fetch request
    if(patIt != patterns_.end())
    {
        negativeReply(*interest, 402);
        return;
    }

    // Else try to create specified pattern
    shared_ptr<IFetchPattern> pattern = patternFactory_.create(
        patternName, getFace(), getKeyChain(),
        bind(static_cast<void(StorageEngine::*)(const ndn::Data&)>(&StorageEngine::put),
             &getStorageHandle(), _1));

    // If pattern can not be created, 
    // Result code: 404:=No such pattern is known
    if(pattern == nullptr)
    {
        negativeReply(*interest, 404);
        return;
    }

    pattern->fetch(fetchPrefix);
    patterns_[fetchPrefix] = pattern;
    this->onDataInsertion(fetchPrefix);

    // reply with only status code, not negative though
    negativeReply(*interest, 100);
}

void PatternHandle::onCancelRequest(const std::shared_ptr<const ndn::Name> &prefix,
                                    const std::shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
                                    uint64_t interestFilterId,
                                    const std::shared_ptr<const ndn::InterestFilter> &filter)
{
    ndn_message::RepoCommandParameterMessage parameter;
    try{
        extractParameter(*interest, *prefix, parameter);
    }
    catch (std::exception& e){
        negativeReply(*interest, 403);
        return;
    }

    Name fetchPrefix;
    for(size_t i = 0; i < parameter.repo_command_parameter().name().component_size(); i ++){
        fetchPrefix.append(parameter.repo_command_parameter().name().component(i));
    }

    auto patIt = patterns_.find(fetchPrefix);
    // If p is found, 
    // Result code: 404:=No such process is in progress.
    if(patIt == patterns_.end())
    {
        negativeReply(*interest, 402);
        return;
    }

    // Cancel the pattern & delete it
    patIt->second->cancel();
    patterns_.erase(patIt);

    // reply with only status code, not negative though
    negativeReply(*interest, 101);
}