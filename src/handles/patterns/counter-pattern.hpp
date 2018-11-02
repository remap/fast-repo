//
// counter-pattern.hpp
//
//  Created by Xinyu Ma on 30 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#ifndef __counter_pattern_hpp__
#define __counter_pattern_hpp__

#include "base-pattern.hpp"

namespace fast_repo
{

/**
 * A simple fetch pattern, for test.
 * Try to fetch the prefix/<n>/, n from 0 to 9.
 * Schedule a next interest 2s after getting a data back.
 * TODO: Refactor FetchPattern class
 */
class CounterPattern : public BasePattern
{
private:
    bool running_;
    int counter_;
    ndn::Name fetchPrefix_;

public:
    CounterPattern(ndn::Face& face, ndn::KeyChain & keyChain, StoreData storePacketFun):
        BasePattern(face, keyChain, storePacketFun), running_(false)
    { 
    }

    static const ndn::Name getPatternKeyword()
    {
        return "counter";
    }

    static std::shared_ptr<IFetchPattern> create(ndn::Face& face, 
                                                 ndn::KeyChain & keyChain, 
                                                 StoreData storePacketFun)
    {
        return std::make_shared<CounterPattern>(face, keyChain, storePacketFun);
    }

    void cancel() override
    {
        // TODO: Improve implementation (in a real pattern)
        running_ = false;
    }

    // I think there will be little chance to give a different face, keyChain or storePacketFun
    // Maybe we can change the interface?
    void fetch(const ndn::Name &prefix) override;

    void doFetch();

    void onData(const std::shared_ptr<const ndn::Interest>& interest,
                const std::shared_ptr<ndn::Data>& data);
};

} // namespace fast_repo

#endif