//
// fast-repo.cpp
//
//  Created by Peter Gusev on 16 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "fast-repo.hpp"

#include <fstream>
#include <boost/property_tree/info_parser.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ndn-cpp/name.hpp>
#include <ndn-cpp/face.hpp>

#include "../storage/storage-engine.hpp"

using namespace fast_repo;
using namespace ndn;

static Config DefaultConfig = Config();

namespace fast_repo
{
class FastRepoImpl : public boost::enable_shared_from_this<FastRepoImpl>
{
  public:
    FastRepoImpl(boost::asio::io_service &io,
                 const Config &config,
                 const boost::shared_ptr<ndn::Face> &face,
                 const boost::shared_ptr<ndn::KeyChain> &keyChain);
    ~FastRepoImpl();

    void enableListening();
    void enableValidation();
    void initializeStorage();

  private:
    boost::asio::io_service &io_;
    Config config_;
    boost::shared_ptr<ndn::Face> face_;
    boost::shared_ptr<ndn::KeyChain> keyChain_;
    boost::shared_ptr<StorageEngine> storageEngine_;

    repo_ng::ReadHandle readHandle_;
#if 0
    repo::WriteHandle writeHandle_;
    repo::WatchHandle watchHandle_;
    repo::DeleteHandle deleteHandle_;
#endif
    PatternHandle patternHandle_;
};
} // namespace fast_repo

Config
fast_repo::parseConfig(const std::string &configPath)
{
    if (configPath.empty())
    {
        std::cerr << "configuration file path is empty" << std::endl;
    }

    std::ifstream fin(configPath.c_str());
    if (!fin.is_open())
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to open configuration file '" + configPath + "'"));

    using namespace boost::property_tree;
    ptree propertyTree;
    try
    {
        read_info(fin, propertyTree);
    }
    catch (const ptree_error &e)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to read configuration file '" + configPath + "'"));
    }

    ptree repoConf = propertyTree.get_child("repo");

    Config repoConfig;
    repoConfig.repoConfigPath = configPath;

    ptree dataConf = repoConf.get_child("data");
    for (const auto &section : dataConf)
    {
        if (section.first == "prefix")
            repoConfig.dataPrefixes.push_back(Name(section.second.get_value<std::string>()));
        else if (section.first == "registration-subset")
            repoConfig.registrationSubset = section.second.get_value<int>();
        else
            BOOST_THROW_EXCEPTION(std::runtime_error("Unrecognized '" + section.first + "' option in 'data' section in "
                                                                                        "configuration file '" +
                                                     configPath + "'"));
    }

    ptree commandConf = repoConf.get_child("command");
    for (const auto &section : commandConf)
    {
        if (section.first == "prefix")
            repoConfig.repoPrefixes.push_back(Name(section.second.get_value<std::string>()));
        else
            BOOST_THROW_EXCEPTION(std::runtime_error("Unrecognized '" + section.first + "' option in 'command' section in "
                                                                                        "configuration file '" +
                                                     configPath + "'"));
    }

    if (repoConf.get<std::string>("storage.method") != "rocksdb")
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Only 'rocksdb' storage method is supported"));
    }

    repoConfig.readOnly = (repoConf.get<std::string>("storage.mode") == "read_only");
    repoConfig.dbPath = repoConf.get<std::string>("storage.path");
    repoConfig.validatorNode = repoConf.get_child("validator");

    return repoConfig;
}

//***
FastRepo::FastRepo(boost::asio::io_service &io,
                   const Config &config,
                   const boost::shared_ptr<ndn::Face> &face,
                   const boost::shared_ptr<ndn::KeyChain> &keyChain)
    : pimpl_(boost::make_shared<FastRepoImpl>(io, config, face, keyChain))
{
    pimpl_->initializeStorage();
}

void FastRepo::enableListening()
{
    pimpl_->enableListening();
}

void FastRepo::enableValidation()
{
    pimpl_->enableValidation();
}

//***
FastRepoImpl::FastRepoImpl(boost::asio::io_service &io,
                           const Config &config,
                           const boost::shared_ptr<ndn::Face> &face,
                           const boost::shared_ptr<ndn::KeyChain> &keyChain)
    : io_(io), config_(config), face_(face), keyChain_(keyChain), storageEngine_(boost::make_shared<StorageEngine>(config_.dbPath, config_.readOnly)), readHandle_(*face_, *storageEngine_, *keyChain_), patternHandle_(*face_, *storageEngine_, *keyChain_)

{
}

FastRepoImpl::~FastRepoImpl()
{
    // TODO: unregister prefixes
}

void FastRepoImpl::enableListening()
{
    for (const ndn::Name &cmdPrefix : config_.repoPrefixes)
    {
        face_->registerPrefix(cmdPrefix,
                              [](const boost::shared_ptr<const Name> &prefix,
                                 const boost::shared_ptr<const Interest> &interest,
                                 Face &face, uint64_t, const boost::shared_ptr<const InterestFilter> &) {
                                     std::cerr << "unexpected interest received: " << interest->getName() << std::endl;
                              },
                              [](const boost::shared_ptr<const Name> &cmdPrefix) {
                                  std::cerr << "failed to register prefix " << cmdPrefix << std::endl;
                                  BOOST_THROW_EXCEPTION(std::runtime_error("Command prefix registration failed"));
                              });

        // m_writeHandle.listen(cmdPrefix);
        // m_watchHandle.listen(cmdPrefix);
        // m_deleteHandle.listen(cmdPrefix);
        patternHandle_.listen(cmdPrefix);
    }
}

void FastRepoImpl::enableValidation()
{
}

void FastRepoImpl::initializeStorage()
{
    std::cout << "opened storage in " << (config_.readOnly ? "readonly" : "read-write")
              << " mode at " << config_.dbPath << std::endl;

    boost::shared_ptr<FastRepoImpl> me = shared_from_this();
    storageEngine_->scanForLongestPrefixes(io_, [me, this](const std::vector<ndn::Name> &prefixes) {
        for (auto p : prefixes)
            config_.dataPrefixes.push_back(Name(p));
        for (auto p : config_.dataPrefixes)
        {
            readHandle_.listen(p);
            std::cout << "registered data prefix: " << p << std::endl;
        }
    });
}