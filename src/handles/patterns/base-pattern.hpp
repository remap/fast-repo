//
// base-pattern.hpp
//
//  Created by Xinyu Ma on 31 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#ifndef __base_pattern_hpp__
#define __base_pattern_hpp__

#include "../../config.hpp"
#include <ndn-cpp/name.hpp>
#include <map>

namespace ndn
{
class Face;
class KeyChain;
class InterestFilter;
} 

namespace fast_repo
{

typedef std::function<void(const ndn::Data &)> StoreData;

/**
 * IFetchPattern is an abstract class for implementation of concrete fetch 
 * pattern classes, like generalized-object, g.o. stream or ndnrtc-stream.
 */
class IFetchPattern
{
  public:
    /**
   * Must return pattern's keyword which will be used by PatternHandle to
   * uniquely identify fetch patterns.
   */
    //virtual const ndn::Name::Component getPatternKeyword() const = 0;

    /**
     * When PatternHandle receives fetch request for this pattern, it will call
     * this method.
     * @param prefix Fetch prefix, retrieved from the fetch request
     */
    virtual void fetch(const ndn::Name &prefix) = 0;

    /**
     * This method might be called by PatternHandle in cases when (ongoing) 
     * fetching must be cancelled immediately.
     */
    virtual void cancel() = 0;

    virtual ~IFetchPattern() = default;
};

/**
 * BasePattern is the base class for implementation of concrete fetch 
 * pattern classes, like generalized-object, g.o. stream or ndnrtc-stream.
 */
class BasePattern : public IFetchPattern
{
  protected:
    ndn::Face & face_;
    ndn::KeyChain & keyChain_;
    StoreData storePacketFun_;

  public:
    /**
     * The constructor of BasePattern
     * @param face Face object
     * @param keyChain KeyChain object
     * @param storePacketFun Must be called by fetch pattern implementations for 
     *                       storing received data packets.
     */
    BasePattern(ndn::Face& face, ndn::KeyChain & keyChain, StoreData storePacketFun):
        face_(face), keyChain_(keyChain), storePacketFun_(storePacketFun){}

    ~BasePattern() override{}
};

typedef std::function<std::shared_ptr<IFetchPattern>(ndn::Face&,ndn::KeyChain&,StoreData)> CreatePattern;

class PatternFactory
{
  private:
    PatternFactory();
    static std::shared_ptr<PatternFactory> instance_;
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
    std::shared_ptr<IFetchPattern> create(ndn::Name name, 
                                          ndn::Face& face, 
                                          ndn::KeyChain& keyChain, 
                                          StoreData storePacketFun);
};

} // namespace fast_repo

#endif