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
import grpc
from google.protobuf import empty_pb2

from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.waterfall_pb2 import Message


def gen():
    for i in range(10):
        yield Message(payload="Message: {}".format(i))
        time.sleep(0.1)


# Create a client
stub = get_default_emulator().get_waterfall_service()

# Let's do some echoing..
it = stub.Echo(gen())
try:
    for msg in it:
        print("Echo: {}".format(msg.payload))
except grpc._channel._Rendezvous as err:
    print(err)
