#  Copyright (C) 2020 The Android Open Source Project
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
from google.protobuf import empty_pb2
from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.emulator_controller_pb2 import DisplayConfigurations, DisplayConfiguration

# Create a client
stub = get_default_emulator().get_emulator_controller()
print("--- Initial state:")
print(stub.getDisplayConfigurations(empty_pb2.Empty()))
print("--- Erase all:")
stub.setDisplayConfigurations(DisplayConfigurations(displays=[]))
print(stub.getDisplayConfigurations(empty_pb2.Empty()))
print("--- Set to two:")
stub.setDisplayConfigurations(DisplayConfigurations(displays=[DisplayConfiguration(width=480, height=720, dpi=142), DisplayConfiguration(width=720, height=1280, dpi=213)]))
print(stub.getDisplayConfigurations(empty_pb2.Empty()))
print("--- Set to one:")
stub.setDisplayConfigurations(DisplayConfigurations(displays=[DisplayConfiguration(width=720, height=1280, dpi=213)]))
print(stub.getDisplayConfigurations(empty_pb2.Empty()))
print("--- Erase all:")
stub.setDisplayConfigurations(DisplayConfigurations(displays=[]))
print(stub.getDisplayConfigurations(empty_pb2.Empty()))
