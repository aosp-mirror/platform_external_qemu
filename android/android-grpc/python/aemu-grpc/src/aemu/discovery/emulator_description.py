# -*- coding: utf-8 -*-
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
from enum import Enum
from typing import Optional

import grpc
import psutil
from aemu.discovery.header_manipulator_client_interceptor import (
    header_adder_interceptor,
)

try:
    # This will fail if you do not have tink..
    from aemu.discovery.security.jwt_header_interceptor import jwt_header_interceptor

    _HAVE_TINK = True
except ImportError:
    _HAVE_TINK = False


from aemu.proto.emulator_controller_pb2 import VmRunState
from aemu.proto.emulator_controller_pb2_grpc import EmulatorControllerStub


_LOGGER = logging.getLogger("aemu-grpc")


def _safe_kill(process: psutil.Process) -> bool:
    """Tries to kill the given process

    Args:
        process (psutil.Process): The process to be terminated

    Returns:
        bool: True if the process is no longer alive.
    """
    status = psutil.STATUS_DEAD
    try:
        status = process.status()
        process.name()  # Retrieve name if possible.
    except Exception as e:
        # Process has left, or we might have been unable to get additional info.
        _LOGGER.debug("Unable to get status, process dead? %s", e)

    if status == psutil.STATUS_ZOMBIE:
        # confirm that the zombie process has been reaped
        try:
            # the process is a zombie, so we need to wait for the parent to reap it
            parent_pid = process.ppid()
            parent = psutil.Process(parent_pid)

            # wait for the parent to reap the zombie process
            parent.wait(timeout=1)
        except Exception as err:
            # well, well, well.. Someone just disappeared on us..
            # or we failed to wait out the reaping. It will get
            # cleaned up later on
            _LOGGER.info("Failed to reap zombie, ignoring %s", err)

        if not psutil.pid_exists(process.pid):
            _LOGGER.info("Process %s has been reaped.", process)

    # Step 1, be nice.
    try:
        _LOGGER.debug("Terminate %s", process)
        process.terminate()
    except Exception as e:
        # Process might be gone, or we could not send terminate.
        _LOGGER.debug("Failed to terminate %s due to %s", process, e)

    try:
        process.wait(timeout=3)
    except psutil.TimeoutExpired:
        _LOGGER.debug("Force kill %s", process)
        process.kill()

    if not psutil.pid_exists(process.pid):
        _LOGGER.info("Successfully terminated: %s", process)

    return not psutil.pid_exists(process.pid)


def _kill_process_tree(process: psutil.Process) -> None:
    """
    Kills the process tree rooted at the given process.

    Args:
        process: The root process of the process tree to kill.
    """
    children = process.children()
    for child in children:
        _LOGGER.debug("Found child %s, terminating..", child)
        _kill_process_tree(child)

    _safe_kill(process)


class EmulatorSecurity(Enum):
    Nothing = 1
    Jwt = 2
    Token = 3


class BasicEmulatorDescription(dict):
    """A description of an emulator you can connect to using gRPC."""

    MAX_MESSAGE_LENGTH = -1

    def __init__(self, description_dict):
        """A BasicEmulatorDescription should have at least the following
        properties:

        - grpc.port or grpc.address
        - Optionally grpc.token
        - Optionally grpc.jwks and grpc.jwks_active

        Currently only produces insecure channels.

        Args:
            description_dict (_type_): _description_
        """
        self._description = description_dict

    def _get_async_channel(self, addr, channel_arguments):
        pass

    def _select_security(self, arguments):
        if "emulator.security" in arguments:
            choice = arguments.get("emulator.security").lower()
            if choice == "jwt":
                _LOGGER.debug("Selecting JWT security")
                return EmulatorSecurity.Jwt
            if choice == "token":
                _LOGGER.debug("Selecting token security")
                return EmulatorSecurity.Token
            _LOGGER.debug("Selecting no security")
            return EmulatorSecurity.Nothing

        if ("grpc.jwks" in self._description) and (
            "grpc.jwk_active" in self._description
        ):
            _LOGGER.debug("Defaulting to JWT security")
            return EmulatorSecurity.Jwt

        if "grpc.token" in self._description:
            _LOGGER.debug("Defaulting to token security")
            return EmulatorSecurity.Token

        _LOGGER.debug("Defaulting to no security")
        return EmulatorSecurity.Nothing

    def get_grpc_channel(self, channel_arguments=[]):
        """Gets a configured gRPC channel to the emulator.

           This will setup all the necessary security credentials, and tokens
           if needed.

        Args:
            channel_arguments: A list of key-value pairs to configure the underlying
            gRPC Core channel or server object. Channel arguments are meant for advanced
            usages and contain experimental API (some may not labeled as experimental).
            Full list of available channel arguments and documentation can be found
            under the “grpc_arg_keys” section of “channel_arg_names.h” header file
            (https://github.com/grpc/grpc/blob/v1.60.x/include/grpc/impl/channel_arg_names.h).

            You can set "emulator.jwks.issuer" to the issuer of the JWT token that will
            be used if JWT is enabled in the emulator. If this is not set it will
            default to PyModule.

            You must make sure the the emulator security configuration
            found in $ANDROID_SDK_ROOT/emulator/lib/emulator_access.json contains
            the appropriate access rules.

            You can force security by setting "emulator.security" to any of:
            ["Nothing", "Token", "Jwt"] otherwise it will use this preference ordering
            based on discovered capabilities:

            Jwt > Token > Nothing

        Returns:
            A fully configured gRPC channel to the emulator.
        """
        port = self.get("grpc.port", 8554)
        addr = self.get("grpc.address", f"localhost:{port}")
        args_as_dict = dict(channel_arguments)
        if "grpc.max_send_message_length" not in args_as_dict:
            channel_arguments.append(
                ("grpc.max_send_message_length", self.MAX_MESSAGE_LENGTH)
            )
        if "grpc.max_receive_message_length" not in args_as_dict:
            channel_arguments.append(
                ("grpc.max_receive_message_length", self.MAX_MESSAGE_LENGTH)
            )

        channel = grpc.insecure_channel(
            addr,
            options=channel_arguments,
        )

        security = self._select_security(args_as_dict)

        if security == EmulatorSecurity.Jwt:
            issuer = args_as_dict.get("emulator.jwks.issuer", "PyModule")
            # We need to create jwks..
            key_path = self._description["grpc.jwks"]
            active_path = self._description["grpc.jwk_active"]
            if not _HAVE_TINK:
                raise NotImplementedError(
                    "Tink is not avaible, cannot provide a secure channel."
                )
            return grpc.intercept_channel(
                channel,
                jwt_header_interceptor(key_path, active_path, False, issuer=issuer),
            )

        if security == EmulatorSecurity.Token:
            bearer = "Bearer {}".format(self.get("grpc.token", ""))
            _LOGGER.debug("Insecure Channel with token to: %s", addr)
            return grpc.intercept_channel(
                channel,
                header_adder_interceptor("authorization", bearer, False),
            )

        return channel

    def get_async_grpc_channel(self, channel_arguments=[]) -> grpc:
        """Gets a configured gRPC asynchronous channel to the emulator.

           This will setup all the necessary security credentials, and tokens
           if needed.

         Args:
            channel_arguments: A list of key-value pairs to configure the underlying
            gRPC Core channel or server object. Channel arguments are meant for advanced
            usages and contain experimental API (some may not labeled as experimental).
            Full list of available channel arguments and documentation can be found
            under the “grpc_arg_keys” section of “channel_arg_names.h” header file
            (https://github.com/grpc/grpc/blob/v1.60.x/include/grpc/impl/channel_arg_names.h).

            You can set "emulator.jwks.issuer" to the issuer of the JWT token that will
            be used if JWT is enabled in the emulator. If this is not set it will
            default to PyModule.

            You must make sure the the emulator security configuration
            found in $ANDROID_SDK_ROOT/emulator/lib/emulator_access.json contains
            the appropriate access rules.

            You can force security by setting "emulator.security" to any of:
            ["Nothing", "Token", "Jwt"] otherwise it will use this preference ordering
            based on discovered capabilities:

            Jwt > Token > Nothing


        Returns:
            A fully configured gRPC async channel to the emulator.
        """
        port = self.get("grpc.port", 8554)
        addr = self.get("grpc.address", f"localhost:{port}")
        args_as_dict = dict(channel_arguments)
        if "grpc.max_send_message_length" not in args_as_dict:
            channel_arguments.append(
                ("grpc.max_send_message_length", self.MAX_MESSAGE_LENGTH)
            )
        if "grpc.max_receive_message_length" not in args_as_dict:
            channel_arguments.append(
                ("grpc.max_receive_message_length", self.MAX_MESSAGE_LENGTH)
            )

        security = args_as_dict.get("emulator.security", "none")
        interceptors = []

        security = self._select_security(args_as_dict)

        if security == EmulatorSecurity.Jwt:
            issuer = args_as_dict.get("emulator.jwks.issuer", "PyModule")

            # We need to create jwks..
            key_path = self._description["grpc.jwks"]
            active_path = self._description["grpc.jwk_active"]
            if not _HAVE_TINK:
                raise NotImplementedError(
                    "Tink is not avaible, cannot provide a secure channel."
                )
            interceptors = jwt_header_interceptor(
                key_path,
                active_path,
                use_async=True,
                issuer=issuer,
            )
        elif security == EmulatorSecurity.Token:
            bearer = "Bearer {}".format(self.get("grpc.token", ""))
            _LOGGER.debug("Insecure async channel with token to: %s", addr)
            interceptors = header_adder_interceptor("authorization", bearer, True)

        return grpc.aio.insecure_channel(
            addr,
            options=channel_arguments,
            interceptors=interceptors,
        )

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


class EmulatorDescription(BasicEmulatorDescription):
    """A description of a (possibly) locally running emulator.

    The description comes from the parsed discovery file.
    """

    def __init__(self, pid, description_dict):
        super().__init__(description_dict)
        self._description["pid"] = pid

    def __str__(self):
        alive = '🚀' if self.is_alive() else '🪦'
        return f"{self.name()} (pid: {self.pid()} {alive})"

    def process(self) -> Optional[psutil.Process]:
        """Returns the process object associated with the qemu process.

        Returns:
            Optional[psutil.Process]: The psutil process object, or one if not running
        """
        procs = [p for p in psutil.process_iter(["pid"]) if p and p.pid == self.pid()]
        return next(iter(procs), None)

    def is_alive(self) -> bool:
        """Checks to see if this emulator description is still alive.

        A Liveness check is done by determining if the pid is still alive, and its
        status is not zombie or dead.

        Returns:
            bool: True if the pid associated with this description is alive
        """
        if not psutil.pid_exists(self.pid()):
            _LOGGER.debug("pid: %s does not exist", self.pid())
            return False

        try:
            status = psutil.Process(self.pid()).status()
            _LOGGER.debug("pid: %s status: %s", self.pid(), status)
            return status != psutil.STATUS_DEAD and status != psutil.STATUS_ZOMBIE
        except Exception as err:
            _LOGGER.debug("pid: %s, failed to retrieve status: %s", self.pid(), err)
            return False

    def _terminate_proc(self, proc, timeout):
        try:
            stub = EmulatorControllerStub(self.get_grpc_channel())
            _LOGGER.debug("Sending shutdown to %s.", self.pid())
            stub.setVmState(VmRunState(state=VmRunState.SHUTDOWN))
        except Exception as err:
            _LOGGER.error("Failed to shutdown using gRPC (%s), terminating.", err)
            proc.terminate()

        try:
            proc.wait(timeout=timeout)
        except psutil.TimeoutExpired as expired:
            _LOGGER.error(
                "Emulator did not shutdown gracefully, terminating. (%s)", expired
            )
            _kill_process_tree(proc)

    def shutdown(self, timeout=30) -> bool:
        """Gracefully shutdown the emulator.

        This sends the shutdown signal to the emulator over gRPC.
        The emulator will be terminated if:

        - The emulator does not shutdown in timeout seconds.
        - The gRPC endpoint is not responding

        Args:
            timeout (int, optional): Timeout before forcefully terminating the emulator. Defaults to 30.

        Returns:
            bool: True if the emulator is no longer running.
        """
        proc = self.process()
        if not proc:
            _LOGGER.debug("No process found, terminated.")
            return True

        try:
            self._terminate_proc(proc, timeout)
        except psutil.NoSuchProcess as no:
            _LOGGER.debug("Process was terminated. (%s)", no)
        except Exception as err:
            _LOGGER.debug(
                "Unexpected error whil terminating process. %s", err, exc_info=True
            )

        return not self.is_alive()

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
