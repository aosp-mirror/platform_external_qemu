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
from ast import Not
import logging

import grpc
from aemu.discovery.header_manipulator_client_interceptor import (
    header_adder_interceptor,
)

try:
    # This will fail if you do not have tink..
    from aemu.discovery.security.jwt_header_interceptor import jwt_header_interceptor

    _HAVE_TINK = True
except ImportError:
    _HAVE_TINK = False


from aemu.proto.emulated_bluetooth_pb2_grpc import EmulatedBluetoothServiceStub
from aemu.proto.emulated_bluetooth_vhci_pb2_grpc import VhciForwardingServiceStub
from aemu.proto.emulator_controller_pb2_grpc import EmulatorControllerStub
from aemu.proto.rtc_service_pb2_grpc import RtcStub
from aemu.proto.snapshot_service_pb2_grpc import SnapshotServiceStub
from aemu.proto.ui_controller_service_pb2_grpc import UiControllerStub
from aemu.proto.waterfall_pb2_grpc import WaterfallStub


class EmulatorDescription(dict):
    """A description of a (possibly) running emulator.

    The description comes from the parsed discovery file.
    """

    def __init__(self, pid, description_dict):
        self._description = description_dict
        self._description["pid"] = pid

    def get_grpc_channel(self, use_async: bool = False) -> grpc.aio.Channel:
        """Gets a configured gRPC channel to the emulator.

           This will setup all the necessary security credentials, and tokens
           if needed.

        Args:
            use_async (bool, optional): True if this should be an async channel.
            Defaults to False.

        Returns:
            grpc.aio.Channel: A fully configured gRPC channel to the emulator.
        """

        # This should default to max
        MAX_MESSAGE_LENGTH = -1

        addr = "localhost:{}".format(self.get("grpc.port", 8554))
        if use_async:
            channel = grpc.aio.insecure_channel(
                addr,
                options=[
                    ("grpc.max_send_message_length", MAX_MESSAGE_LENGTH),
                    ("grpc.max_receive_message_length", MAX_MESSAGE_LENGTH),
                ],
            )
        else:
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

        if ("grpc.jwks" in self._description) and (
            "grpc.jwk_active" in self._description
        ):
            # We need to create jwks..
            key_path = self._description["grpc.jwks"]
            active_path = self._description["grpc.jwk_active"]
            if not _HAVE_TINK:
                raise NotImplementedError(
                    "Tink is not avaible, cannot provide a secure channel."
                )
            return grpc.intercept_channel(
                channel, jwt_header_interceptor(key_path, active_path)
            )

        logging.debug("Insecure channel to %s", addr)
        return channel

    def get_async_grpc_channel(self) -> grpc.aio.Channel:
        """Gets an asynchronous channel to the emulator.

        Returns:
            grpc.aio.Channel: An asynchronous connection the emulator.
        """
        return self.get_grpc_channel(True)

    def get_emulator_controller(
        self, use_async: bool = False
    ) -> EmulatorControllerStub:
        """Gets an EmulatorControllerStub to the emulator

        Args:
            use_async (bool, optional): True if this should be async. Defaults to False.

        Returns:
            EmulatorControllerStub: An RPC stub to the emulator controller.
        """
        channel = self.get_grpc_channel(use_async)
        return EmulatorControllerStub(channel)

    def get_snapshot_service(self) -> SnapshotServiceStub:
        """Returns a stub to the snapshot service."""
        return SnapshotServiceStub(self.get_grpc_channel())

    def get_waterfall_service(self, use_async=False) -> WaterfallStub:
        """Returns a stub to the waterfall service.

        Note: Requires an image with a waterfall service installed,
        or an emulator running with a waterfall extension.
        """
        channel = self.get_grpc_channel(use_async)
        return WaterfallStub(channel)

    def get_rtc_service(self, use_async=False) -> RtcStub:
        """Returns a stub to the Rtc service.

        Note: Requires an emulator with WebRTC support. Usually this
        means you are running the emulator under linux.
        """
        channel = self.get_grpc_channel(use_async)
        return RtcStub(channel)

    def get_ui_controller_service(self, use_async=False) -> UiControllerStub:
        """Returns a stub to the Ui Controller service."""
        channel = self.get_grpc_channel(use_async)
        return UiControllerStub(channel)

    def get_vhci_forwarder_service(self, use_async=False) -> VhciForwardingServiceStub:
        """Returns a stub to the Vhci Forwarding  service."""
        channel = self.get_grpc_channel(use_async)
        return VhciForwardingServiceStub(channel)

    def get_emulated_bluetooth_service(
        self, use_async=False
    ) -> EmulatedBluetoothServiceStub:
        """Returns a stub to the emulated bluetooth service."""
        channel = self.get_grpc_channel(use_async)
        return EmulatedBluetoothServiceStub(channel)

    def name(self) -> str:
        """Returns the name of the emulator as displayed by adb.

        You can use this name with adb using the -s flag,
        or this name can be set as the ANDROID_SERIAL environment variable.

        Returns:
            str: The name of the emulator, as displayed by adb
        """
        serial = self._description["port.serial"]
        return f"emulator-{serial}"

    def pid(self) -> int:
        """Gets the pid of the emulator.

        Returns:
            int: The process id of the emulator.
        """
        return int(self.get("pid"))

    def get(self, prop: str, default_value: str = None) -> str:
        """Gets the property from the emulator description

        Args:
            prop (str): The property from the discovery file
            default_value (str, optional): Default value if the
                    property does not exist. Defaults to None.

        Returns:
            str: _description_
        """
        return self._description.get(prop, default_value)

    def __hash__(self):
        return hash(frozenset(self._description.items()))
