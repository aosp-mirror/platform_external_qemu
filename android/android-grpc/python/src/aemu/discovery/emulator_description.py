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
import logging

import grpc

from aemu.discovery.header_manipulator_client_interceptor import (
    header_adder_interceptor,
)
from aemu.proto.emulator_controller_pb2_grpc import EmulatorControllerStub
from aemu.proto.snapshot_service_pb2_grpc import SnapshotServiceStub
from aemu.proto.waterfall_pb2_grpc import WaterfallStub
from aemu.proto.rtc_service_pb2_grpc import RtcStub
from aemu.proto.ui_controller_service_pb2_grpc import UiControllerStub


class EmulatorDescription(dict):
    """A description of a (possibly) running emulator.

       The description comes from the parsed discovery file.
    """

    def __init__(self, pid, description_dict):
        self._description = description_dict
        self._description["pid"] = pid

    def get_grpc_channel(self):
        """Gets a gRPC channel to the emulator

           It will inject the proper tokens when needed.

           Note: Currently only produces insecure channels.
        """
        # This should default to max
        MAX_MESSAGE_LENGTH = -1

        addr = "localhost:{}".format(self.get("grpc.port", 8554))
        channel = grpc.insecure_channel(
            addr,
            options=[
                ("grpc.max_send_message_length", MAX_MESSAGE_LENGTH),
                ("grpc.max_receive_message_length", MAX_MESSAGE_LENGTH),
            ],
        )
        if "grpc.token" in self._description:
            bearer = "Bearer {}".format(self.get("grpc.token", ""))
            logging.debug("Insecure Channel with token to: %s", addr)
            return grpc.intercept_channel(
                channel, header_adder_interceptor("authorization", bearer)
            )

        logging.debug("Insecure channel to %s", addr)
        return channel

    def get_emulator_controller(self):
        """Returns a stub to the emulator controller service."""
        return EmulatorControllerStub(self.get_grpc_channel())

    def get_snapshot_service(self):
        """Returns a stub to the snapshot service."""
        return SnapshotServiceStub(self.get_grpc_channel())

    def get_waterfall_service(self):
        """Returns a stub to the waterfall service.

        Note: Requires an image with a waterfall service installed,
        or an emulator running with a waterfall extension.
        """
        return WaterfallStub(self.get_grpc_channel())

    def get_rtc_service(self):
        """Returns a stub to the Rtc service.

        Note: Requires an emulator with WebRTC support. Usually this
        means you are running the emulator under linux.
        """
        return RtcStub(self.get_grpc_channel())

    def get_ui_controller_service(self):
        """Returns a stub to the Ui Controller service."""
        return UiControllerStub(self.get_grpc_channel())

    def name(self):
        """Returns the name of the emulator.

        You can use this name with adb using the -s flag,
        or this name can be set as the ANDROID_SERIAL environment variable.
        """
        return "emulator-{}".format(self._description["port.serial"])

    def pid(self):
        """Returns the pid of this emulator."""
        return self.get("pid")

    def get(self, prop, default_value=None):
        """Gets a property from the emulator description."""
        return self._description.get(prop, default_value)

    def __hash__(self):
        return hash(frozenset(self._description))
