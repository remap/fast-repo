//
// pattern-factory.cpp
//
//  Created by Peter Gusev on 3 December 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "pattern-factory.hpp"

#include "counter-pattern.hpp"
#include "ndnrtc-pattern.hpp"

using namespace ndn;
using namespace fast_repo;
using namespace std;
using namespace std::placeholders;
using boost::shared_ptr;

boost::shared_ptr<PatternFactory> PatternFactory::instance_;

PatternFactory::PatternFactory()
{
    patterns_[CounterPattern::getPatternKeyword()] = &CounterPattern::create;
#if HAVE_LIBNDNRTC
    patterns_[NdnrtcPattern::getPatternKeyword()] = &NdnrtcPattern::create;
#endif
}

PatternFactory& PatternFactory::getInstance()
{
    if(!instance_){
        instance_ = shared_ptr<PatternFactory>(new PatternFactory());
    }
    return *instance_;
}

boost::shared_ptr<IFetchPattern> PatternFactory::create(Name name, 
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