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
import sys

from google.protobuf import empty_pb2

import proto.snapshot_service_pb2
import proto.snapshot_service_pb2_grpc
from channel.channel_provider import getEmulatorChannel

if len(sys.argv) != 2:
    print("You need to provide the id of the snapshot to pull")
    sys.exit(1);


_EMPTY_ = empty_pb2.Empty()

channel = getEmulatorChannel()

# Create a client
stub = proto.snapshot_service_pb2_grpc.SnapshotServiceStub(channel)
snaphot = proto.snapshot_service_pb2.SnapshotPackage(snapshot_id=sys.argv[1])
it = stub.PullSnapshot(snaphot)
try:
    with open(sys.argv[1] +'.tar.gz', 'wb') as fn:
        for msg in it:
            if not msg.success:
                print("Failed to pull snapshot: " + msg.err)

            fn.write(msg.payload)
except grpc._channel._Rendezvous as err:
    print(err)
