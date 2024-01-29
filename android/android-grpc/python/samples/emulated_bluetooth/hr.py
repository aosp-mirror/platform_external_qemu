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
"""
A sample of how you would build an emulated heart rate monitor.
"""
import logging
import time
from concurrent import futures

import grpc
from aemu.discovery.emulator_discovery import EmulatorDiscovery, get_default_emulator
from aemu.proto.emulated_bluetooth_device_pb2 import (
    Advertisement,
    CharacteristicValueResponse,
    GattCharacteristic,
    GattDevice,
    GattProfile,
    GattService,
    Uuid,
)
from aemu.proto.emulated_bluetooth_pb2_grpc import EmulatedBluetoothServiceStub
from aemu.proto.emulated_bluetooth_device_pb2_grpc import (
    GattDeviceServiceServicer,
    add_GattDeviceServiceServicer_to_server,
)
from aemu.proto.grpc_endpoint_description_pb2 import Endpoint
from google.protobuf.empty_pb2 import Empty
from google.protobuf import text_format


class HeartRateMonitor(GattDeviceServiceServicer):
    # Well known heart rate monitor short uuids
    HRS_MEASUREMENT_UUID = 0x2A37
    HRS_BODY_SENSOR_LOC_UUID = 0x2A38
    HRS_CONTROL_POINT = 0x2A39
    HRS_UUID = 0x180D
    DEVICE_INFO_UUID = 0x180A
    MANUFACTURER_NAME_UUID = 0x2A29
    MODEL_NUMBER_UUID = 0x2A24

    def __init__(self):
        self.sensor_location = 0x01  # Chest..
        self.model_nr = "Snake Runner"
        self.manufacture_name = "ACME Rattler & Co"

        # The response we will give to read requests.
        self.response_map = {
            HeartRateMonitor.HRS_BODY_SENSOR_LOC_UUID: bytes([self.sensor_location]),
            HeartRateMonitor.MANUFACTURER_NAME_UUID: self.manufacture_name.encode(),
            HeartRateMonitor.MODEL_NUMBER_UUID: self.model_nr.encode(),
        }

    def heartbeatUpdater(self):
        """An iteratator that yields a heart rate measurement every second."""
        hr = 92
        while True:
            yield CharacteristicValueResponse(
                data=bytes(
                    [
                        0x06,  # Sensor contact + 8 bit hr value.
                        hr,
                    ]
                ),
            )

            # Increment the heart beat.
            hr = hr + 1
            if hr > 160:
                hr = 92
            time.sleep(1.0)

    def OnCharacteristicReadRequest(self, request, context):
        """Developers must implement this to provide a read response."""

        logging.info(
            "OnCharacteristicReadRequest: %s",
            text_format.MessageToString(request, as_one_line=True),
        )
        return CharacteristicValueResponse(
            data=self.response_map[request.callback_id.id]
        )

    def OnCharacteristicObserveRequest(self, request, context):
        """Developers must provide this to register an observer."""

        logging.info(
            "OnCharacteristicObserveRequest: %s",
            text_format.MessageToString(request, as_one_line=True),
        )
        # Technically we should only receive a request with this uuid
        assert request.callback_id.id == HeartRateMonitor.HRS_MEASUREMENT_UUID
        return self.heartbeatUpdater()

    def OnCharacteristicWriteRequest(self, request, context):
        """Developers must implement this to handle a write."""

        logging.info(
            "OnCharacteristicWriteRequest: %s",
            text_format.MessageToString(request, as_one_line=True),
        )
        # Reset Energy Expended: resets the value of the Energy Expended field
        # in the Heart Rate Measurement characteristic to 0
        assert request.callback_id.id == HeartRateMonitor.HRS_CONTROL_POINT
        return CharacteristicValueResponse()

    def OnConnectionStateChange(self, request, context):
        """Developers must implement this to handle a connection change."""
        logging.info(
            "OnConnectionStateChange: %s",
            text_format.MessageToString(request, as_one_line=True),
        )
        return Empty()


def register():
    """Registers the heart rate monitor with the first discovered emulator."""
    server = start_server()
    stub = EmulatedBluetoothServiceStub(
        get_default_emulator([("emulator.security", "token")]).get_grpc_channel()
    )

    device_description = GattDevice(
        # Location where I registered my server, the nimble stack will
        # call this endpoint for information.
        endpoint=Endpoint(target="localhost:50051"),
        # Some set of advertising parameters, these are additional
        # advertising parameters
        advertisement=Advertisement(device_name="Acme Heart Rate Monitor"),
        # The profile I have to offer.
        profile=GattProfile(
            services=[
                # The set of services I provide.
                GattService(
                    # The actual heart rate monitor..
                    uuid=Uuid(id=HeartRateMonitor.HRS_UUID),
                    characteristics=[
                        GattCharacteristic(
                            uuid=Uuid(id=HeartRateMonitor.HRS_MEASUREMENT_UUID),
                            properties=GattCharacteristic.Properties.PROPERTY_NOTIFY,
                        ),
                        GattCharacteristic(
                            uuid=Uuid(id=HeartRateMonitor.HRS_BODY_SENSOR_LOC_UUID),
                            properties=GattCharacteristic.Properties.PROPERTY_READ,
                        ),
                        GattCharacteristic(
                            uuid=Uuid(id=HeartRateMonitor.HRS_CONTROL_POINT),
                            properties=GattCharacteristic.Properties.PROPERTY_WRITE,
                        ),
                    ],
                ),
                GattService(
                    # General device information.
                    uuid=Uuid(id=HeartRateMonitor.DEVICE_INFO_UUID),
                    characteristics=[
                        GattCharacteristic(
                            uuid=Uuid(id=HeartRateMonitor.MANUFACTURER_NAME_UUID),
                            properties=GattCharacteristic.Properties.PROPERTY_READ,
                        ),
                        GattCharacteristic(
                            uuid=Uuid(id=HeartRateMonitor.MODEL_NUMBER_UUID),
                            properties=GattCharacteristic.Properties.PROPERTY_READ,
                        ),
                    ],
                ),
            ]
        ),
    )

    status = stub.registerGattDevice(device_description)
    logging.info(
        "Registered: %s", text_format.MessageToString(status, as_one_line=True)
    )
    server.wait_for_termination()


def start_server():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    add_GattDeviceServiceServicer_to_server(HeartRateMonitor(), server)
    server.add_insecure_port("[::]:50051")
    server.start()
    return server


def main():
    logging.basicConfig(level=logging.INFO)
    logging.warning(
        "This might require you to run the emulator with the `-packet-streamer-endpoint default` flag."
    )
    register()


if __name__ == "__main__":
    main()
