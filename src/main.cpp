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

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <ndn-cpp/threadsafe-face.hpp>
#include <ndn-cpp/security/key-chain.hpp>

#include "../third_party/docopt/docopt.h"
#include "config.hpp"
#include "repo/fast-repo.hpp"

using namespace std;
using namespace ndn;

static const char USAGE[] =
    R"(Fast Repo.

    Usage:
      fast-repo  --config=<config_file> [ --db-path=<path_to_db> | --verbose ]

    Options:
      -c --config=<config_file>     Config file
      -d --db-path=<path_to_db>     Path to KV storage folder [ default: /tmp/fast-repo ]
      -v --verbose                  Verbose output
)";

void terminate(boost::asio::io_service &ioService,
               const boost::system::error_code &error,
               int signalNo,
               boost::asio::signal_set &signalSet)
{
    if (error)
        return;

    ioService.stop();

    if (signalNo == SIGABRT || signalNo == SIGSEGV)
    {
        std::cout << "Caught signal '" << strsignal(signalNo) << "', exiting..." << std::endl;

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
    signalSet.async_wait(std::bind(static_cast<void(*)(boost::asio::io_service&,const boost::system::error_code&,int,boost::asio::signal_set&)>(&terminate), 
                                   std::ref(ioService),
                                   std::placeholders::_1, std::placeholders::_2,
                                   std::ref(signalSet)));

    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
                                                               {argv + 1, argv + argc},
                                                               true,
                                                               (string("Fast Repo ") + string(PACKAGE_VERSION)).c_str());

    // for(auto const& arg : args) {
    //     std::cout << arg.first << " " <<  arg.second << std::endl;
    // }

    boost::shared_ptr<Face> face = boost::make_shared<ThreadsafeFace>(ioService);
    boost::shared_ptr<KeyChain> keyChain = boost::make_shared<KeyChain>();

    fast_repo::FastRepo repoInstance(ioService, 
                                     fast_repo::parseConfig(args["--config"].asString()), 
                                     face, keyChain);

    ioService.run();

    return 0;
}
