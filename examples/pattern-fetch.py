#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (C) 2014-2018 Regents of the University of California.
# Author: Jeff Thompson <jefft0@remap.ucla.edu>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# A copy of the GNU Lesser General Public License is in the file COPYING.

import time
from sys import argv
from pyndn import Name
from pyndn import Data
from pyndn import Interest
from pyndn import Face
from pyndn.security import KeyChain
from pyndn.encoding import ProtobufTlv

import repo_command_response_pb2
import repo_command_parameter_pb2

PATTERN_NAME = Name("counter")

def dump(*list):
    result = ""
    for element in list:
        result += (element if type(element) is str else str(element)) + " "
    print(result)

def requestInsert(face, repoCommandPrefix, fetchName, onInsertStarted, onFailed):
    # Construct a RepoCommandParameterMessage using the structure in
    # repo_command_parameter_pb2 which was produced by protoc.
    parameter = repo_command_parameter_pb2.RepoCommandParameterMessage()
    # Add the Name.
    msgName = parameter.Name()

    # 1. pattern name
    msgName.Clear()
    msgName.component.append(PATTERN_NAME[0].getValue().toBytes())
    parameter.repo_command_parameter.name.component.append(ProtobufTlv.encode(msgName).toBytes())

    # 2. fetch prefix
    msgName.Clear()
    for compo in fetchName:
        msgName.component.append(compo.getValue().toBytes())
    parameter.repo_command_parameter.name.component.append(ProtobufTlv.encode(msgName).toBytes())

    # Create the command interest.
    interest = Interest(Name(repoCommandPrefix).append("pattern")
        .append(Name.Component(ProtobufTlv.encode(parameter))))
    face.makeCommandInterest(interest)

    # Send the command interest and get the response or timeout.
    def onData(interest, data):
        # repo_command_response_pb2 was produced by protoc.
        response = repo_command_response_pb2.RepoCommandResponseMessage()
        try:
            ProtobufTlv.decode(response, data.content)
        except:
            dump("Cannot decode the repo command response")
            onFailed()

        if response.repo_command_response.status_code == 100:
            onInsertStarted()
        else:
            dump("Got repo command error code", response.repo_command_response.status_code)
            onFailed()

    def onTimeout(interest):
        dump("Pattern repo command timeout")
        onFailed()

    face.expressInterest(interest, onData, onTimeout)

def main():
    if len(argv) < 3:
        dump("Usage:", argv[0], "<repo-command-prefix> <fetch-prefix>")
        return

    repoCommandPrefix = Name(argv[1])
    repoDataPrefix = Name(argv[2])

    # The default Face will connect using a Unix socket, or to "localhost".
    face = Face()
    # Use the system default key chain and certificate name to sign commands.
    keyChain = KeyChain()
    face.setCommandSigningInfo(keyChain, keyChain.getDefaultCertificateName())

    enabled = [True]
    def onInsertStarted():
        # nonlocal enabled
        dump("Insert started for", repoDataPrefix.toUri())
        enabled[0] = False
    def onFailed():
        # nonlocal enabled
        enabled[0] = False
    requestInsert(face, repoCommandPrefix, repoDataPrefix, onInsertStarted, onFailed)

    # Run until all the data is sent.
    while enabled[0]:
        face.processEvents()
        # We need to sleep for a few milliseconds so we don't use 100% of the CPU.
        time.sleep(0.01)

    face.shutdown()

main()
