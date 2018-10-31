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
{
    addPattern(make_shared<CounterPattern>());
}

void PatternHandle::listen(const ndn::Name &prefix)
{
    getFace().setInterestFilter(ndn::Name(prefix).append("pattern"),
                                bind(&PatternHandle::onInterest, this, _1, _2, _3, _4, _5));
}

void PatternHandle::addPattern(shared_ptr<IFetchPattern> p)
{
    patterns_[p->getPatternKeyword()] = p;
}

void PatternHandle::removePattern(shared_ptr<IFetchPattern> p)
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

    shared_ptr<IFetchPattern> p;
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
                 bind(static_cast<void(StorageEngine::*)(const ndn::Data&)>(&StorageEngine::put), 
                      &getStorageHandle(), _1));
    }else{
        // This should be impossible
        throw Error("Pattern pool is broken.");
    }
}