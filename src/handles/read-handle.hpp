/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018, Regents of the University of California.
 *
 * This file is part of NDN repo-ng (Next generation of NDN repository).
 * See AUTHORS.md for complete list of repo-ng authors and contributors.
 *
 * repo-ng is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * repo-ng is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * repo-ng, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REPO_HANDLES_READ_HANDLE_HPP
#define REPO_HANDLES_READ_HANDLE_HPP

#include "../config.hpp"
#include "base-handle.hpp"

namespace ndn
{
class InterestFilter;
}

namespace repo_ng
{

class ReadHandle : public BaseHandle
{
  public:
    //   using DataPrefixRegistrationCallback = std::function<void(const ndn::Name&)>;
    //   using DataPrefixUnregistrationCallback = std::function<void(const ndn::Name&)>;
    //   struct RegisteredDataPrefix
    //   {
    //     const ndn::RegisteredPrefixId* prefixId;
    //     int useCount;
    //   };

    ReadHandle(ndn::Face &face, RepoStorage &storageHandle, ndn::KeyChain &keyChain);

    void
    listen(const ndn::Name &prefix) override;

    // PUBLIC_WITH_TESTS_ELSE_PRIVATE:
    //   const std::map<ndn::Name, RegisteredDataPrefix>&
    //   getRegisteredPrefixes()
    //   {
    //     return m_insertedDataPrefixes;
    //   }

    // TODO: repo-ng had callbacks on the storage to notify about inserted/deleted
    // data packets. This was used to track use count on the prefixes and unregister
    // or register them if needed.
    // I don't think we shall use this functionality for fast-repo as there will be
    // thousandss of prefixes to watch.
#if 0
  /**
   * @param after Do something after actually removing a prefix
   */
  void
  onDataDeleted(const Name& name);

  /**
   * @param after Do something after successfully registering the data prefix
   */
  void
  onDataInserted(const Name& name);

  void
  connectAutoListen();
#endif

  private:
    /**
   * @brief Read data from backend storage
   */
    void
    onInterest(const std::shared_ptr<const ndn::Name> &prefix,
               const std::shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
               uint64_t interestFilterId,
               const std::shared_ptr<const ndn::InterestFilter> &filter);

    void
    onRegisterFailed(const std::shared_ptr<const ndn::Name> &prefix);

  private:
    //   size_t m_prefixSubsetLength;
    //   std::map<ndn::Name, RegisteredDataPrefix> m_insertedDataPrefixes;
    //   ndn::util::signal::ScopedConnection afterDataDeletionConnection;
    //   ndn::util::signal::ScopedConnection afterDataInsertionConnection;
};

} // namespace repo_ng

#endif // REPO_HANDLES_READ_HANDLE_HPP
