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
from google.protobuf import empty_pb2

import sys
import proto.snapshot_service_pb2
import proto.snapshot_service_pb2_grpc
import proto.waterfall_pb2
import proto.waterfall_pb2_grpc
from channel.channel_provider import getEmulatorChannel

# Open a grpc channel
channel = getEmulatorChannel()

_EMPTY_ = empty_pb2.Empty()

# Create a client
snap = proto.snapshot_service_pb2_grpc.SnapshotServiceStub(channel)

target = proto.snapshot_service_pb2.IceboxTarget(processName=sys.argv[1])
response = snap.trackProcess(target)
print(response)
