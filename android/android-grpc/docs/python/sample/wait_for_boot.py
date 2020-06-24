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
import grpc
from google.protobuf import empty_pb2

from aemu.discovery.emulator_discovery import get_default_emulator

# Create a client
stub = get_default_emulator().get_emulator_controller()

# Poll for boot complete every sec..
for i in range(0, 60):
    time.sleep(1)
    try:
        response = stub.getStatus(empty_pb2.Empty())
        if response.booted:
            print("Emulator ready")
            break
    except grpc._channel._Rendezvous as err:
        # Assume grpc endpoint is not yet up..
        pass
