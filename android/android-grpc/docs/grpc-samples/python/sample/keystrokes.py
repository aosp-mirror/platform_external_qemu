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
import os

_EMPTY_ = empty_pb2.Empty()


def getChannel():
  """Opens up a grpc an (in)secure channel to localhost.

  If a cert is available in ~/.android then
  this expects that the emulator is using a self signed cert:
  made as follows:

  openssl req -x509 -out localhost.crt -keyout localhost.key \
      -newkey rsa:2048 -nodes -sha256 \
      -subj '/CN=localhost' -extensions EXT -config <( \
     printf "[dn]\nCN=localhost\n[req]\ndistinguished_name = dn\n[EXT]\nsubjectAltName=DNS:localhost\nkeyUsage=digitalSignature\nextendedKeyUsage=serverAuth")
  """

  cert = os.path.join(os.path.expanduser("~"), '.android', 'emulator-grpc.cer')
  if os.path.exists(cert):
    with open(cert, 'rb') as f:
      creds = grpc.ssl_channel_credentials(f.read())
      channel = grpc.secure_channel('localhost:5556', creds)
  else:
    # Let's go insecure..
    channel = grpc.insecure_channel('localhost:5556')



  return channel

channel = getChannel()

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
