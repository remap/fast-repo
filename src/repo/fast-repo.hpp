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

    std::string repoConfigPath;
    std::string dbPath;
    std::vector<ndn::Name> dataPrefixes;
    size_t registrationSubset = DISABLED_SUBSET_LENGTH;
    std::vector<ndn::Name> repoPrefixes;
    uint64_t nMaxPackets;
    boost::property_tree::ptree validatorNode;
};

Config
parseConfig(const std::string&);

class FastRepo
{
  public:
    FastRepo(boost::asio::io_service &io,
             const Config &config,
             const boost::shared_ptr<ndn::Face> &face,
             const boost::shared_ptr<ndn::KeyChain> &keyChain);

    void enableListening();
    void enableValidation();

  private:
    boost::asio::io_service &io_;
    Config config_;
    boost::shared_ptr<ndn::Face> face_;
    boost::shared_ptr<ndn::KeyChain> keyChain_;
    boost::shared_ptr<StorageEngine> storageEngine_;
};

} // namespace fast_repo

#endif