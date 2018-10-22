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

PatternHandle::PatternHandle(ndn::Face &face, StorageEngine &storage, ndn::KeyChain &keyChain)
    : BaseHandle(face, storage, keyChain)
{
}

void PatternHandle::listen(const ndn::Name &prefix)
{
    getFace().setInterestFilter(InterestFilter(prefix),
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

void PatternHandle::onInterest(const std::shared_ptr<const ndn::Name> &prefix,
                               const std::shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
                               uint64_t interestFilterId,
                               const std::shared_ptr<const ndn::InterestFilter> &filter)
{
    // TODO: implement pattern-selection logic
    std::shared_ptr<IFetchPattern> p; // = <select pattern from available>

    if (p) // if found - start fetching
    {
        // TODO: extract fetch prefix from the request
        Name fetchPrefix; // = extractPatternFetchPrefix(interest);
        p->fetch(getFace(), getKeyChain(), fetchPrefix,
                 std::bind(static_cast<void(StorageEngine::*)(const ndn::Data&)>(&StorageEngine::put), 
                      &getStorageHandle(), std::placeholders::_1));
    } // else -- send NetworkNack
    else
    {
        // TODO: send network nack
    }
}