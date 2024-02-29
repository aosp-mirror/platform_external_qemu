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
import os
import platform
import re
from pathlib import Path
from typing import Optional

from aemu.discovery.emulator_description import (
    BasicEmulatorDescription,
    EmulatorDescription,
)


_LOGGER = logging.getLogger("aemu-grpc")

class EmulatorNotFound(Exception):
    pass


class EmulatorDiscovery(object):
    """A Class used to discover all running emulators on the system."""

    ANDROID_SUBDIR = ".android"

    # Format of our discovery files.
    _PID_FILE_ = re.compile("pid_(\\d+).ini")

    def __init__(self):
        self.discovery_dirs = get_discovery_directories()

    def _discover_running(self) -> set[EmulatorDescription]:
        """Discovers all the running emulators by parsing the
        available discovery files.
        """
        emulators = set()
        for discovery_dir in self.discovery_dirs:
            _LOGGER.debug("Discovering emulators in %s", discovery_dir)
            if discovery_dir.exists():
                for file in discovery_dir.glob("*.ini"):
                    pid_file = self._PID_FILE_.match(file.name)
                    if pid_file:
                        _LOGGER.debug("Found %s", file)
                        try:
                            emu = EmulatorDescription(
                                pid_file.group(1), self._parse_ini(file)
                            )
                            if emu.is_alive():
                                emulators.add(emu)
                        except Exception as err:
                            _LOGGER.error("Failed to parse %s due to %s, ignoring", file, err)

        return emulators

    def _parse_ini(self, ini_file: str) -> dict[str, str]:
        """Parse a discovered ini file

        Args:
            ini_file (str): The emulator description file

        Returns:
            dict[str, str]: The dictionary containing the emulator information.
        """
        _LOGGER.debug("Discovering emulator: %s", ini_file)
        emu = dict()
        with open(ini_file, mode="r", encoding="utf-8") as ini:
            for line in ini.readlines():
                line = line.strip()
                if "=" in line:
                    isi = line.index("=")
                    emu[line[:isi]] = line[isi + 1 :]
        return emu

    def available(self) -> int:
        """The number of discovered emulators.

        Returns:
            int: The number of discovered emulators.
        """
        return len(self._discover_running())

    def emulators(self) -> set[EmulatorDescription]:
        """All the running emulators

        Returns:
            set[EmulatorDescription]: All discovered emulators.
        """
        return frozenset(self._discover_running())

    def find_emulator(self, prop: str, value: str) -> EmulatorDescription:
        """Finds the emulator where the given property has the given value.

        Args:
            prop (str): The property we are looking for.
            value (str): The value of the property

        Returns:
            EmulatorDescription: The discovered emulator or None if no such emulator
        """
        for emu in self._discover_running():
            if emu.get(prop) == value:
                return emu

        return None

    def find_by_pid(self, pid: int) -> Optional[EmulatorDescription]:
        """Finds the emulator with the give process id.

        Args:
            pid (int): The process id of the running emulator.

        Returns:
            Optional[EmulatorDescription]: The discovered emulator, or None
                                           if no such emulator was found.
        """
        return self.find_emulator("pid", str(pid))

    def first(self) -> EmulatorDescription:
        """Gets the first discovered emulator.

        Raises:
            EmulatorNotFound: No running emulators were found.

        Returns:
            EmulatorDescription: The first discovered running emulator.
        """
        emulators = self._discover_running()
        if len(emulators) == 0:
            raise EmulatorNotFound("No running emulators were found.")
        return next(iter(emulators))

    @staticmethod
    def connection(
        address: str, token: Optional[str] = None
    ) -> BasicEmulatorDescription:
        """Creates a connection to an emulator based upon a uri
        and token.

        You usually want to use this if you want to establish a connection
        to a remote emulator.

        Args:
            address (str): The endpoint you wish to connect to
            token (Optional[str]): Token to use, if any
        """
        description = {"grpc.address": address}
        if token:
            description["grpc.token"] = token
        return BasicEmulatorDescription(description)


def get_discovery_directories() -> list[Path]:
    """All the discovery directories that can contain emulator discovery files.

    Returns:
        list[Path]: A list of directories with possible discovery files.
    """
    path = None
    if platform.system() == "Windows" and "LOCALAPPDATA" in os.environ:
        path = Path(os.environ.get("LOCALAPPDATA")) / "Temp"
    if platform.system() == "Linux":
        if "XDG_RUNTIME_DIR" in os.environ:
            path = Path(os.environ.get("XDG_RUNTIME_DIR"))
        if path is None or not path.exists():
            path = Path("/") / "run" / "user" / Path(str(os.getuid()))
    if platform.system() == "Darwin" and "HOME" in os.environ:
        path = Path.home() / "Library" / "Caches" / "TemporaryItems"

    paths = _get_user_directories()
    paths.append(path)
    return [path / "avd" / "running" for path in paths if path != None]


def _get_user_directories():
    # See ConfigDirs::getUserDirectory()
    _LOGGER.debug("Retrieving user directories")
    paths = []

    if "ANDROID_EMULATOR_HOME" in os.environ:
        paths.append(Path(os.environ.get("ANDROID_EMULATOR_HOME")))

    if "ANDROID_SDK_HOME" in os.environ:
        paths.append(
            Path(os.environ.get("ANDROID_SDK_HOME")) / EmulatorDiscovery.ANDROID_SUBDIR
        )

    if "ANDROID_AVD_HOME" in os.environ:
        paths.append(Path(os.environ.get("ANDROID_AVD_HOME")))

    paths.append(Path.home() / EmulatorDiscovery.ANDROID_SUBDIR)
    return paths


def get_default_emulator() -> EmulatorDescription:
    """The first discovered emulator.

        Useful if you expect only one running emulator.

    Raises:
        EmulatorNotFound: No running emulators were found.

    Returns:
        EmulatorDescription: The first discovered emulator.
    """
    return EmulatorDiscovery().first()
