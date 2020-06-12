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
import os
import sys
import platform
import grpc
from urllib.parse import urlparse
from . import header_manipulator_client_interceptor


class Emulators(object):
    """This class discovers all the running emulators by parsing the various discovery files."""

    def __init__(self):
        if platform.system() == "Windows":
            path = os.path.join(os.environ.get("LOCALAPPDATA"), "Temp")
        if platform.system() == "Linux":
            path = os.environ.get("XDG_RUNTIME_DIR")
        if platform.system() == "Darwin":
            path = os.path.join(
                os.environ.get("HOME"), "Library", "Caches", "TemporaryItems"
            )

        self.discovery_dir = os.path.join(path, "avd", "running")
        self.emulators = {}
        for file in os.listdir(self.discovery_dir):
            if file.endswith(".ini"):
                emu = self._parse_ini(os.path.join(self.discovery_dir, file))
                self.emulators[emu.get("port.serial", "0")] = emu

    def _parse_ini(self, ini_file):
        """Parse an emulator ini file."""
        emu = {}
        with open(ini_file, "r") as ini:
            for line in ini.readlines():
                line = line.strip()
                if "=" in line:
                    isi = line.index("=")
                    emu[line[:isi]] = line[isi + 1 :]
        return emu

    def _is_uri(self, maybe_uri):
        """True if maybe_uri is a uri."""
        try:
            result = urlparse(maybe_uri)
            return all([result.scheme, result.netloc, result.path])
        except:
            return False

    def getDefaultChannel(self):
        """Gets a channel to the first discovered emulator."""
        return self.getEmulatorChannel(next(iter(self.emulators)))

    def getEmulatorChannel(self, emu_id_or_uri=None):
        """Gets a gRPC channel to the emulator, identified by:

            emu_id_or_uri: A uri to a gRPC endpoint.
            emu_id_or_uri: Console port of the running emulator.
            emu_id_or_uri: None, the first discovered emulator

            It will inject the proper tokens when needed.
        """

        if not emu_id_or_uri:
            emu_id_or_uri = next(iter(self.emulators))

        if self._is_uri(emu_id_or_uri):
            return grpc.insecure_channel(emu_id_or_uri)

        emu = self.emulators[emu_id_or_uri]
        addr = "localhost:{}".format(emu["grpc.port"])
        channel = grpc.insecure_channel(addr)
        if "grpc.token" in emu:
            bearer = "Bearer {}".format(emu["grpc.token"])
            header_adder_interceptor = header_manipulator_client_interceptor.header_adder_interceptor(
                "authorization", bearer
            )
            return grpc.intercept_channel(channel, header_adder_interceptor)

        return channel


def getEmulatorChannel():
    """Gets a proper gRPC channel to the first discovered emulator."""
    return Emulators().getDefaultChannel()
