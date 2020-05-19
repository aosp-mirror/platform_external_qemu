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
import os
import sys


import proto.snapshot_service_pb2
import proto.snapshot_service_pb2_grpc
from channel.channel_provider import getEmulatorChannel

if len(sys.argv) != 2:
    print("You need to provide the id of the snapshot to push")
    sys.exit(1);

def read_in_chunks(file_object, chunk_size=(128*1024)):
    while True:
        data = file_object.read(chunk_size)
        if not data:
            break
        yield data


def push_snapshot(fname):
    snap_id  = os.path.basename(fname).replace('.tar.gz', '')
    yield proto.snapshot_service_pb2.SnapshotPackage(snapshot_id=snap_id)
    with open(fname, 'rb') as snap:
        for chunk in read_in_chunks(snap):
            yield proto.snapshot_service_pb2.SnapshotPackage(payload=chunk)


channel = getEmulatorChannel()

if not os.path.exists(sys.argv[1]):
    raise Exception("File " + sys.argv[1] + " does not exist.")

 # Create a client
stub = proto.snapshot_service_pb2_grpc.SnapshotServiceStub(channel)
msg = stub.PushSnapshot(push_snapshot(sys.argv[1]))
print(msg)
