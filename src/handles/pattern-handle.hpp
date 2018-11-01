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
#include "patterns/base-pattern.hpp"

namespace ndn
{
class Face;
class KeyChain;
class InterestFilter;
} 

namespace fast_repo
{

/**
 * A handle for pattern fetching. 
 * Exact pattern fetching implementations must derive from IPatternFetch
 */
class PatternHandle : public repo_ng::BaseHandle
{
  public:
    PatternHandle(ndn::Face &face, StorageEngine &storage, ndn::KeyChain &keyChain);

    void listen(const ndn::Name &prefix);

    void removePattern(const ndn::Name &fetchPrefix);

  private:
    PatternFactory& patternFactory_;

    std::map<ndn::Name, std::shared_ptr<IFetchPattern>> patterns_;

    void onInterest(const std::shared_ptr<const ndn::Name> &prefix,
                    const std::shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
                    uint64_t interestFilterId,
                    const std::shared_ptr<const ndn::InterestFilter> &filter);

    void onCancelRequest(const std::shared_ptr<const ndn::Name> &prefix,
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