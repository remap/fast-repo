//
// pattern-factory.hpp
//
//  Created by Peter Gusev on 3 December 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#ifndef __pattern_factory_hpp__
#define __pattern_factory_hpp__

#include "base-pattern.hpp"

namespace fast_repo {

using boost::shared_ptr;

typedef std::function<shared_ptr<IFetchPattern>(ndn::Face&,ndn::KeyChain&,StoreData)> CreatePattern;

class PatternFactory
{
  private:
    PatternFactory();
    static shared_ptr<PatternFactory> instance_;
    std::map<ndn::Name, CreatePattern> patterns_;

  public:
    static PatternFactory& getInstance();

    /**
     * Create a concrete fetch pattern by name
     * @param name Pattern name
     * @param face Face object
     * @param keyChain KeyChain object
     * @param storePacketFun Must be called by fetch pattern implementations for 
     *                       storing received data packets.
     */
    shared_ptr<IFetchPattern> create(ndn::Name name, 
                                          ndn::Face& face, 
                                          ndn::KeyChain& keyChain, 
                                          StoreData storePacketFun);

    std::vector<ndn::Name> getSupportedPatterns() const;
};

}

#endif