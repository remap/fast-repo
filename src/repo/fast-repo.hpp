//
// fast-repo.hpp
//
//  Created by Peter Gusev on 16 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#ifndef __fast_repo_hpp__
#define __fast_repo_hpp__

#include <vector>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <ndn-cpp/name.hpp>

#include "../handles/read-handle.hpp"
#include "../handles/write-handle.hpp"
#include "../handles/watch-handle.hpp"
#include "../handles/delete-handle.hpp"
#include "../handles/pattern-handle.hpp"


namespace ndn
{
class Face;
class KeyChain;
} // namespace ndn

namespace fast_repo
{

class StorageEngine;

struct Config
{
    static const size_t DISABLED_SUBSET_LENGTH = -1;

    bool readOnly;
    std::string repoConfigPath;
    std::string dbPath;
    std::vector<ndn::Name> dataPrefixes;
    size_t registrationSubset = DISABLED_SUBSET_LENGTH;
    std::vector<ndn::Name> repoPrefixes;
    boost::property_tree::ptree validatorNode;
};

Config
parseConfig(const std::string&);

static Config DefaultConfig;

class FastRepoImpl;
class FastRepo
{
  public:
    FastRepo(boost::asio::io_service &io,
             const Config &config,
             const std::shared_ptr<ndn::Face> &face,
             const std::shared_ptr<ndn::KeyChain> &keyChain);

    void enableListening();
    void enableValidation();

  private:
    std::shared_ptr<FastRepoImpl> pimpl_;
};

} // namespace fast_repo

#endif