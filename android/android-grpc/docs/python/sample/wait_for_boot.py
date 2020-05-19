#  Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# -*- coding: utf-8 -*-
import time

from google.protobuf import empty_pb2

import grpc
import proto.emulator_controller_pb2
import proto.emulator_controller_pb2_grpc
from channel.channel_provider import getEmulatorChannel

_EMPTY_ = empty_pb2.Empty()

channel = getEmulatorChannel()

# Create a client
stub = proto.emulator_controller_pb2_grpc.EmulatorControllerStub(channel)

# Poll for boot complete every sec..
booted = False
for i in range(0, 60):
    time.sleep(1)
    try:
        response = stub.getStatus(_EMPTY_)
        if response.booted:
            print("Emulator ready")
            break
    except grpc._channel._Rendezvous as err:
        # Assume grpc endpoint is not yet up..
        pass
