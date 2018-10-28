//
// pattern-handle.hpp
//
//  Created by Peter Gusev on 17 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#ifndef __pattern_handle_hpp__
#define __pattern_handle_hpp__

#include <ndn-cpp/name.hpp>

#include "base-handle.hpp"

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
    virtual const ndn::Name::Component getPatternKeyword() const = 0;

    /**
     * When PatternHandle receives fetch request for this pattern, it will call
     * this method.
     * @param face Face object
     * @param keyChain KeyChain object
     * @param prefix Fetch prefix, retrieved from the fetch request
     * @param storePacketFun Must be called by fetch pattern implementations for 
     *                       storing received data packets.
     */
    virtual void fetch(ndn::Face &, ndn::KeyChain &,
                       const ndn::Name &prefix, StoreData storePacketFun) = 0;

    /**
     * This method might be called by PatternHandle in cases when (ongoing) 
     * fetching must be cancelled immediately.
     */
    virtual void cancel() = 0;
};

/**
 * A handle for pattern fetching. 
 * Exact pattern fetching implementations must derive from IPatternFetch
 */
class PatternHandle : public repo_ng::BaseHandle
{
  public:
    PatternHandle(ndn::Face &face, StorageEngine &storage, ndn::KeyChain &keyChain);

    void listen(const ndn::Name &prefix);

    void addPattern(std::shared_ptr<IFetchPattern> p);
    void removePattern(std::shared_ptr<IFetchPattern> p);

  private:
    std::map<ndn::Name::Component, std::shared_ptr<IFetchPattern>> patterns_;

    void onInterest(const std::shared_ptr<const ndn::Name> &prefix,
                    const std::shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
                    uint64_t interestFilterId,
                    const std::shared_ptr<const ndn::InterestFilter> &filter);

  private:
    bool decodeNames(const ndn_message::RepoCommandParameterMessage_Name &composed,
                     ndn::Name &patternName,
                     ndn::Name &fetchPrefix);
};

} // namespace fast_repo

#endif