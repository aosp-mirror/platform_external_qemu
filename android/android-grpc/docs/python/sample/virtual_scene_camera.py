#  Copyright (C) 2021 The Android Open Source Project
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
import math

from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.emulator_controller_pb2 import RotationRadian
from aemu.proto.emulator_controller_pb2 import Velocity

PI = math.pi
# Create a client
stub = get_default_emulator().get_emulator_controller()
#turn left and turn back
stub.rotateVirtualSceneCamera(RotationRadian(x = 0.0, y = PI/2, z = 0.0))
time.sleep(1)
stub.rotateVirtualSceneCamera(RotationRadian(x = 0.0, y = -PI/2, z = 0.0))
time.sleep(1)
#turn right and turn back
stub.rotateVirtualSceneCamera(RotationRadian(x = 0.0, y = -PI/2, z = 0.0))
time.sleep(1)
stub.rotateVirtualSceneCamera(RotationRadian(x = 0.0, y = PI/2, z = 0.0))
time.sleep(1)
#move forward and move backward
stub.setVirtualSceneCameraVelocity(Velocity(x = 0.0, y = 0.0, z = -1.0))
time.sleep(1)
stub.setVirtualSceneCameraVelocity(Velocity(x = 0.0, y = 0.0, z = 1.0))
time.sleep(1)
#stop
stub.setVirtualSceneCameraVelocity(Velocity(x = 0.0, y = 0.0, z = 0.0))
