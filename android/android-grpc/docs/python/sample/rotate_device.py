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

from google.protobuf import empty_pb2

import proto.emulator_controller_pb2 as p
import proto.emulator_controller_pb2_grpc
from channel.channel_provider import getEmulatorChannel
import time

# Open a grpc channel
channel = getEmulatorChannel()

# Create a client
stub = proto.emulator_controller_pb2_grpc.EmulatorControllerStub(channel)

# Rotate the emulator around..
rotation = [-180, -90, 0, 90]
for i in rotation:
    model = p.PhysicalModelValue(
        target=p.PhysicalModelValue.ROTATION, value=p.ParameterValue(data=[0, 0, i])
    )
    stub.setPhysicalModel(model)
    print(stub.getPhysicalModel(model))
    time.sleep(2)
