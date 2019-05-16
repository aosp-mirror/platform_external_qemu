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
import grpc
import time
from . import emulator_controller_pb2
from . import emulator_controller_pb2_grpc
from google.protobuf import empty_pb2

_EMPTY_ = empty_pb2.Empty()

# Open a grpc channel
channel = grpc.insecure_channel('localhost:5556')

# Create a client
stub = emulator_controller_pb2_grpc.EmulatorControllerStub(channel)

# Let's ask about the hypervisor.
response = stub.getVmConfiguration(_EMPTY_)
print(response)

# Let's type some text..
for l in "Hello World":
    textEvent = emulator_controller_pb2.KeyboardEvent(text=l)
    print("Typing: {}".format(l))
    response = stub.sendKey(textEvent)
    time.sleep(0.2)
