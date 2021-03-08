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
from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.emulator_controller_pb2 import Touch, TouchEvent


# Create a client
stub = get_default_emulator().get_emulator_controller()


# Sample sequence used to debug some multi touch issues.
stub.sendTouch(TouchEvent(touches=[Touch(x=661, y=1133, pressure=12, identifier=2)]))
stub.sendTouch(TouchEvent(touches=[Touch(x=661, y=1133, pressure=1)]))
stub.sendTouch(TouchEvent(touches=[Touch(x=335, y=940, identifier=1, pressure=1)]))
stub.sendTouch(TouchEvent(touches=[Touch(x=203, y=823, identifier=0)]))
stub.sendTouch(TouchEvent(touches=[Touch(x=203, y=823, identifier=1)]))
stub.sendTouch(TouchEvent(touches=[Touch(x=801, y=1281, identifier=2)]))
