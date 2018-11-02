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

#include "read-handle.hpp"

#include <ndn-cpp/face.hpp>

using namespace ndn;

using std::shared_ptr;
using std::make_shared;
using std::bind;

namespace repo_ng
{

ReadHandle::ReadHandle(Face &face, RepoStorage &storageHandle, KeyChain &keyChain)
    : BaseHandle(face, storageHandle, keyChain)
{
    // connectAutoListen();
}

void
ReadHandle::connectAutoListen()
{
  // Connect a RepoStorage's signals to the read handle
  // NOTE : This function is only used in test.
  // Delete it from the constructor on real use.
  getStorageHandle().afterDataInsertion.connect(bind(&ReadHandle::onDataInserted, this, _1));
}

void ReadHandle::onInterest(const shared_ptr<const ndn::Name> &prefix,
                            const shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
                            uint64_t interestFilterId,
                            const shared_ptr<const ndn::InterestFilter> &filter)
{
    shared_ptr<ndn::Data> data = getStorageHandle().read(*interest);
    if (data != nullptr)
    {
        getFace().putData(*data);
    }
    else{
      // TODO: else - sendNetworkNack
    }
}

void ReadHandle::onRegisterFailed(const shared_ptr<const ndn::Name> &prefix)
{
    std::cerr << "ERROR: Failed to register prefix in local hub's daemon" << std::endl;
    getFace().shutdown();
}

void ReadHandle::listen(const Name &prefix)
{
    getFace().registerPrefix(prefix,
                             bind(&ReadHandle::onInterest, this, _1, _2, _3, _4, _5),
                             bind(&ReadHandle::onRegisterFailed, this, _1));
}

void
ReadHandle::onDataInserted(const Name& name)
{
  ndn::InterestFilter filter(name);
  // Note: Used only for test.
  // Different behaviours with "real" run as below.
  // Do not handle dumplicated name
  // Do not modify config.dataPrefixes
  // Do not use longest prefixes
  // this->listen(name);
}

#if 0
void
ReadHandle::onDataDeleted(const Name& name)
{
  // We add one here to account for the implicit digest at the end,
  // which is what we get from the underlying storage when deleting.
  Name prefix = name.getPrefix(-(m_prefixSubsetLength + 1));
  auto check = m_insertedDataPrefixes.find(prefix);
  if (check != m_insertedDataPrefixes.end()) {
    if (--(check->second.useCount) <= 0) {
      getFace().unsetInterestFilter(check->second.prefixId);
      m_insertedDataPrefixes.erase(prefix);
    }
  }
}
#endif

} // namespace repo_ng
