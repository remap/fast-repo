//
// base-pattern.cpp
//
//  Created by Xinyu Ma on 31 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "base-pattern.hpp"

#include "counter-pattern.hpp"
#include <functional>

using namespace ndn;
using namespace fast_repo;
using namespace std;
using namespace std::placeholders;


std::shared_ptr<PatternFactory> PatternFactory::instance_;

PatternFactory::PatternFactory()
{
    patterns_[CounterPattern::getPatternKeyword()] = &CounterPattern::create;
}

PatternFactory& PatternFactory::getInstance()
{
    if(!instance_){
        instance_ = shared_ptr<PatternFactory>(new PatternFactory());
    }
    return *instance_;
}

shared_ptr<IFetchPattern> PatternFactory::create(Name name, 
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