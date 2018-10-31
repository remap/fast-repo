//
// counter-pattern.hpp
//
//  Created by Xinyu Ma on 30 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#ifndef __counter_pattern_hpp__
#define __counter_pattern_hpp__

#include "../pattern-handle.hpp"

namespace fast_repo
{

/**
 * A simple fetch pattern, for test.
 * Try to fetch the prefix/<n>/, n from 0 to 9.
 * Schedule a next interest 2s after getting a data back.
 * TODO: Refactor FetchPattern class
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

    // NOTE: static; used by factory
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
               const ndn::Name &prefix, StoreData storePacketFun) override;

    void doFetch();

    void onData(const std::shared_ptr<const ndn::Interest>& interest,
                const std::shared_ptr<ndn::Data>& data);
};

} // namespace fast_repo

#endif