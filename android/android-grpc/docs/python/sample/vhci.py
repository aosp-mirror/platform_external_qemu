#  Copyright (C) 2022 The Android Open Source Project
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

import time

from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.emulated_bluetooth_packets_pb2 import HCIPacket

# Create a client
stub = get_default_emulator().get_vhci_forwarder_service()


def vhci_stream():
    # Just sleep as we have no actual bluetooth stack here.
    time.sleep(10)

    # Let's give it a bad packet..
    yield HCIPacket(type=HCIPacket.PACKET_TYPE_EVENT, packet="bad".encode())


# This merely dumps out all the incoming HCI packets.
# It is a simple smoke test to confirm that packets are actually
# coming your way as we have no real bluetooth stack to make sense
# of the packets.
print("Dumping incoming HCI Packets for 10 seconds.")
for response in stub.attachVhci(vhci_stream()):
    print("Packet: {}".format(response))
