//
// pattern-factory.cpp
//
//  Created by Peter Gusev on 3 December 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "pattern-factory.hpp"

#include "counter-pattern.hpp"

#if HAVE_LIBNDNRTC
#include "ndnrtc-pattern.hpp"
#endif

#if HAVE_LIBCNL_CPP
#include "gobj-pattern.hpp"
#include "gobj-stream-pattern.hpp"
#endif

using namespace ndn;
using namespace fast_repo;
using namespace std;
using namespace std::placeholders;
using std::shared_ptr;

std::shared_ptr<PatternFactory> PatternFactory::instance_;

PatternFactory::PatternFactory()
{
    patterns_[CounterPattern::getPatternKeyword()] = &CounterPattern::create;
#if HAVE_LIBNDNRTC
    patterns_[NdnrtcPattern::getPatternKeyword()] = &NdnrtcPattern::create;
#endif
#if HAVE_LIBCNL_CPP
    patterns_[GeneralizedObjectPattern::getPatternKeyword()] = &GeneralizedObjectPattern::create;
    patterns_[GeneralizedObjectPattern::getPatternKeyword()] = &GeneralizedObjectPattern::create;
#endif
}

PatternFactory& PatternFactory::getInstance()
{
    if(!instance_){
        instance_ = shared_ptr<PatternFactory>(new PatternFactory());
    }
    return *instance_;
}

std::shared_ptr<IFetchPattern> PatternFactory::create(Name name,
                                                 Face& face,
                                                 KeyChain& keyChain,
                                                 StoreData storePacketFun)
{
    auto it = patterns_.find(name.toUri());
    if(it == patterns_.end()){
        return nullptr;
    }else{
        return it->second(face, keyChain, storePacketFun);
    }
}

vector<Name> PatternFactory::getSupportedPatterns() const
{
    vector<Name> p;
    for (auto it:patterns_)
        p.push_back(it.first);
    return p;
}
