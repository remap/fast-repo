//
// counter-pattern.cpp
//
//  Created by Xinyu Ma on 30 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "counter-pattern.hpp"

#include <ndn-cpp/face.hpp>
#include <ndn-cpp/interest-filter.hpp>
#include <functional>

using namespace fast_repo;
using namespace ndn;

using std::bind;
using boost::shared_ptr;
using boost::make_shared;

void CounterPattern::fetch(const ndn::Name &prefix)
{
    fetchPrefix_ = prefix;
    counter_ = 0;

    face_.callLater(1.0, bind(&CounterPattern::doFetch, this));

    running_ = true;
}

void CounterPattern::doFetch()
{
    if(!running_)
        return;
    ndn::Interest fetchInterest(ndn::Name(fetchPrefix_).append(std::to_string(counter_)));

    fetchInterest.setInterestLifetimeMilliseconds(4000.0);
    face_.expressInterest(fetchInterest, 
                           bind(&CounterPattern::onData, this, _1, _2));
}

void CounterPattern::onData(const shared_ptr<const ndn::Interest>& interest,
                            const shared_ptr<ndn::Data>& data)
{
    // Put data without validation
    storePacketFun_(*data);

    // Schedule the next fetch
    counter_ ++;
    if(counter_ < 10){
        face_.callLater(2000.0, bind(&CounterPattern::doFetch, this));
    }else{
        running_ = false;
    }
}