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
from typing import Optional
from pathlib import Path


from aemu.discovery.emulator_description import EmulatorDescription


class EmulatorDiscovery(object):
    """A Class used to discover all running emulators on the system."""

    ANDROID_SUBDIR = ".android"

    # Format of our discovery files.
    _PID_FILE_ = re.compile("pid_(\\d+).ini")

    def __init__(self):
        self.discovery_dirs = get_discovery_directories()
        self.discover()

    def discover(self) -> None:
        """Discovers all the running emulators by parsing the
        available discovery files.
        """
        self._emulators = set()
        for discovery_dir in self.discovery_dirs:
            logging.debug("Discovering emulators in %s", discovery_dir)
            if discovery_dir.exists():
                for file in discovery_dir.glob("*.ini"):
                    m = self._PID_FILE_.match(file.name)
                    if m:
                        logging.debug("Found %s", file)
                        emu = self._parse_ini(file)
                        if emu:
                            self._emulators.add(EmulatorDescription(m.group(1), emu))

    def _parse_ini(self, ini_file: str) -> dict[str, str]:
        """Parse a discovered ini file

        Args:
            ini_file (str): The emulator description file

        Returns:
            dict[str, str]: The dictionary containing the emulator information.
        """
        logging.debug("Discovering emulator: %s", ini_file)
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
        return len(self._emulators)

    def emulators(self) -> set[EmulatorDescription]:
        """All the running emulators

        Returns:
            set[EmulatorDescription]: All discovered emulators.
        """
        return frozenset(self._emulators)

    def find_emulator(self, prop: str, value: str) -> Optional[EmulatorDescription]:
        """Finds the emulator where the given property has the given value.

        Args:
            prop (str): The property we are looking for.
            value (str): The value of the property

        Returns:
            EmulatorDescription: The discovered emulator or None if not found.
        """
        for emu in self._emulators:
            if emu.get(prop) == value:
                return emu

        return None

    def find_by_pid(self, pid: int) -> Optional[EmulatorDescription]:
        """Finds the emulator with the give process id

        Args:
            pid (int): The process id of the running emulator.

        Returns:
            Optional[EmulatorDescription]: The discovered emulator, or None
                                           if no such emulator was found.
        """
        return self.find_emulator("pid", str(pid))

    def first(self) -> EmulatorDescription:
        """Gets the first discovered emulator."""
        return next(iter(self._emulators))


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
            path = Path("/") / "run" / "user" / Path(os.getuid())
    if platform.system() == "Darwin" and "HOME" in os.environ:
        path = Path.home() / "Library" / "Caches" / "TemporaryItems"

    paths = _get_user_directories()
    paths.append(path)
    return [path / "avd" / "running" for path in paths if path != None]


def _get_user_directories():
    # See ConfigDirs::getUserDirectory()
    logging.debug("Retrieving user directories")
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

    Returns:
        EmulatorDescription: The first discovered emulator.
    """
    return EmulatorDiscovery().first()
