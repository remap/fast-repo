/**
 * Copyright (C) 2018-2019 Regents of the University of California.
 * @author: Peter Gusev <peter@remap.ucla.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version, with the additional exemption that
 * compiling, linking, and/or using OpenSSL is allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * A copy of the GNU Lesser General Public License is in the file COPYING.
 */

#include <map>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include <functional>

#include "../third_party/docopt/docopt.h"
#include "config.hpp"
#include "repo/fast-repo.hpp"

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/log/trivial.hpp>
#include <ndn-cpp/threadsafe-face.hpp>
#include <ndn-cpp/security/key-chain.hpp>

using namespace std;
using namespace ndn;
namespace logging = boost::log;

using std::bind;
using std::ref;

static const char USAGE[] =
    R"(Fast Repo.

    Usage:
      fast-repo  ( --config=<config_file> | ( <command_prefix> [ --db-path=<path_to_db> ] )) [ --rename-prefix=<repo_prefix> ] [ --validate | --readonly | --verbose ]

    Arguments:
      <command_prefix>              Prefix repo must register for incoming commands

    Options:
      -c --config=<config_file>     Config file
      -d --db-path=<path_to_db>     Path to KV storage folder [default: /var/db/fast-repo]
      --validate                    Make repo validate every fetched packet
      --readonly                    Starts repo in read-only mode: repo won't listen for commands, only serve data.
                                    DB will open in read-only mode, thus allowing multiple processes to open at the 
                                    same time.
      --rename-prefix=<repo_prefix>   Optional argument, when present, repo will re-package received data packets into new 
                                    data packets with new names that have this prefix. This is a hack and won't work if 
                                    data is signed. Repo will sign using dummy signature.
      -v --verbose                  Verbose output

    Examples:
      fast-repo --config=repo.conf
      fast-repo /ndn/fast-repo --db-path=$HOME/fast-repo
)";

void init_logging(bool verbose)
{
    if (!verbose)
    {
        // logging::core::get()->set_filter
        // (
        //     logging::trivial::severity >= logging::trivial::info
        // );
    }
}

void terminate(boost::asio::io_service &ioService,
               const boost::system::error_code &error,
               int signalNo,
               boost::asio::signal_set &signalSet)
{
    if (error)
        return;

    std::cout << "Caught signal '" << strsignal(signalNo) << "', exiting..." << std::endl;
    ioService.stop();

    if (signalNo == SIGABRT || signalNo == SIGSEGV)
    {
        void *array[10];
        size_t size;
        size = backtrace(array, 10);
        // print out all the frames to stderr
        backtrace_symbols_fd(array, size, STDERR_FILENO);
    }
}

int main(int argc, char **argv)
{
    boost::asio::io_service ioService;
    boost::asio::signal_set signalSet(ioService);

    signalSet.add(SIGINT);
    signalSet.add(SIGTERM);
    signalSet.add(SIGABRT);
    signalSet.add(SIGSEGV);
    signalSet.add(SIGHUP);
    signalSet.add(SIGUSR1);
    signalSet.add(SIGUSR2);
    signalSet.async_wait(bind(static_cast<void(*)(boost::asio::io_service&,const boost::system::error_code&,int,boost::asio::signal_set&)>(&terminate), 
                              ref(ioService),
                              _1, _2,
                              ref(signalSet)));

    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
                                                               {argv + 1, argv + argc},
                                                               true,
                                                               (string("Fast Repo ") + string(PACKAGE_VERSION)).c_str());

    init_logging(args["--verbose"].asBool());

    for(auto const& arg : args)
        BOOST_LOG_TRIVIAL(trace) << arg.first << " " <<  arg.second;

    std::shared_ptr<Face> face = std::make_shared<ThreadsafeFace>(ioService);
    std::shared_ptr<KeyChain> keyChain = std::make_shared<KeyChain>();

    // TODO: make sure this is setup correctly
    face->setCommandSigningInfo(*keyChain, keyChain->getDefaultCertificateName());

    fast_repo::Config repoConfig = (args["--config"].isString() ? 
                                fast_repo::parseConfig(args["--config"].asString()) :
                                fast_repo::DefaultConfig);
    
    // override db path, if needed
    if (args["<command_prefix>"].isString())
    {
        repoConfig.repoPrefixes.push_back(ndn::Name(args["<command_prefix>"].asString()));
        repoConfig.dbPath = args["--db-path"].asString();
    }
    // override read only mode if needed
    if (args["--readonly"].asBool())
        repoConfig.readOnly = true;

    if (args["--rename-prefix"].isString())
        repoConfig.renamePrefix = args["--rename-prefix"].asString();

    fast_repo::FastRepo repoInstance(ioService, 
                                     repoConfig, 
                                     face, keyChain);
    
    // repo automatically will register prefixes for data
    // listening for commands must be enabled explicitly
    if (!repoConfig.readOnly)
        repoInstance.enableListening();

    if (args["--validate"].asBool())
        repoInstance.enableValidation();

    ioService.run();

    return 0;
}
