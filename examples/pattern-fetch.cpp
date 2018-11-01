/**
 * Copyright (C) 2015-2018 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
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
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <ndn-cpp/face.hpp>
#include <ndn-cpp/data.hpp>
#include <ndn-cpp/interest.hpp>
#include <ndn-cpp/name.hpp>
#include <ndn-cpp/security/key-chain.hpp>
#include <ndn-cpp/encoding/protobuf-tlv.hpp>
#include "../src/repo-command-response.pb.h"
#include "../src/repo-command-parameter.pb.h"

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;
using namespace ndn::ptr_lib;

const std::string PATTERN_NAME = "counter";

typedef function<void()> SimpleCallback;

static void
requestFetchOnData
(const shared_ptr<const Interest>& interest,
 const shared_ptr<Data>& data, const SimpleCallback& onInsertStarted,
 const SimpleCallback& onFailed);

static void
requestFetchOnTimeout
(const shared_ptr<const Interest>& interest,
 const SimpleCallback& onFailed);

void
requestInsert
(Face& face, const Name& repoCommandPrefix, const Name& fetchName,
 const SimpleCallback& onInsertStarted, const SimpleCallback& onFailed,
 int startBlockId = -1, int endBlockId = -1)
{
    // Construct a RepoCommandParameterMessage using the structure in
    // repo-command-parameter.pb.cc which was produced by protoc.
    ndn_message::RepoCommandParameterMessage parameter;
  
    // Add the Name.
    ndn_message::RepoCommandParameterMessage_Name msgName;
  
    // 1. pattern name
    msgName.clear_component();
    msgName.add_component(PATTERN_NAME);
    parameter.mutable_repo_command_parameter()->mutable_name()->add_component
        (ndn::ProtobufTlv::encode(msgName).toRawStr());
  
    // 2. fetch prefix
    msgName.clear_component();
    for (size_t i = 0; i < fetchName.size(); ++i){
        msgName.add_component(fetchName.get(i).getValue().buf(), fetchName.get(i).getValue().size());
    }
    parameter.mutable_repo_command_parameter()->mutable_name()->add_component
        (ndn::ProtobufTlv::encode(msgName).toRawStr());
  
    // Create the command interest.
    Interest interest(Name(repoCommandPrefix).append("pattern")
                      .append(Name::Component(ProtobufTlv::encode(parameter))));
    face.makeCommandInterest(interest);
  
    // Send the command interest and get the response or timeout.
    face.expressInterest
    (interest, bind(&requestFetchOnData, _1, _2, onInsertStarted, onFailed),
     bind(&requestFetchOnTimeout, _1, onFailed));
}

static void
requestFetchOnData
(const ptr_lib::shared_ptr<const Interest>& interest,
 const ptr_lib::shared_ptr<Data>& data, const SimpleCallback& onInsertStarted,
 const SimpleCallback& onFailed)
{
    ndn_message::RepoCommandResponseMessage response;
    try {
        ProtobufTlv::decode(response, data->getContent());
    }
    catch (std::exception& e) {
        cout << "Cannot decode the repo command response " << e.what() << endl;
        onFailed();
    }
  
    if (response.repo_command_response().status_code() == 100)
        onInsertStarted();
    else {
        cout << "Got repo command error code "  <<
        response.repo_command_response().status_code() << endl;
        onFailed();
    }
}

static void
requestFetchOnTimeout
(const ptr_lib::shared_ptr<const Interest>& interest,
 const SimpleCallback& onFailed)
{
    cout << "Pattern repo command timeout" << endl;
    onFailed();
}

/**
 * Print the message (if not empty) and set *enabled = false.
 */
static void
printAndQuit(const string& message, bool *enabled);

static void
onInsertStarted(const Name& fetchPrefix);

int main(int argc, char** argv)
{
    try {
        if(argc < 3){
            cout << "Usage: " << argv[0] << " <repo-command-prefix> <fetch-prefix>" << endl;
            exit(1);
        }
      
        Name repoCommandPrefix(argv[1]);
        Name repoDataPrefix(argv[2]);
      
        Name fetchPrefix = Name(repoDataPrefix);
      
        // The default Face will connect using a Unix socket, or to "localhost".
        Face face;
      
        // Use the system default key chain and certificate name to sign commands.
        KeyChain keyChain;
        face.setCommandSigningInfo(keyChain, keyChain.getDefaultCertificateName());
      
        bool enabled = true;
      
        requestInsert
        (face, repoCommandPrefix, fetchPrefix,
         bind(&onInsertStarted, fetchPrefix),
         // For failure, already printed the error.
         bind(&printAndQuit, "", &enabled));
      
        // Loop calling processEvents until a callback sets enabled = false.
        while (enabled) {
            face.processEvents();
            // We need to sleep for a few milliseconds so we don't use 100% of the CPU.
            usleep(1000);
        }
    } catch (std::exception& e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}

static void
printAndQuit(const string& message, bool *enabled)
{
    if (message != "")
        cout << message << endl;
    *enabled = false;
}

static void
onInsertStarted(const Name& fetchPrefix)
{
    cout << "Fetch started for " << fetchPrefix.toUri() << endl;
    exit(0);
}
