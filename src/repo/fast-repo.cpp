//
// fast-repo.cpp
//
//  Created by Peter Gusev on 16 October 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "fast-repo.hpp"

#include <fstream>
#include <boost/property_tree/info_parser.hpp>
#include <ndn-cpp/name.hpp>

using namespace fast_repo;
using namespace ndn;

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

    repoConfig.dbPath = repoConf.get<std::string>("storage.path");
    repoConfig.validatorNode = repoConf.get_child("validator");

    return repoConfig;
}

FastRepo::FastRepo(boost::asio::io_service &io,
             const Config &config,
             const boost::shared_ptr<ndn::Face> &face,
             const boost::shared_ptr<ndn::KeyChain> &keyChain)
             : io_(io)
             , config_(config)
             , face_(face)
             , keyChain_(keyChain)
{
}

void
FastRepo::enableListening() 
{

}

void
FastRepo::enableValidation()
{

}