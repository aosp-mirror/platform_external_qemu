# -*- coding: utf-8 -*-
# Copyright 2023 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import logging
import re
from pathlib import Path

from toolchain_generator import ToolchainGenerator


def compare_versions(v1: str, v2: str) -> int:
    """Compares two version strings.

    Args:
        v1 (str): The first version string.
        v2 (str): The second version string.

    Returns:
        int: -1 if v1 is older than v2, 0 if v1 is equal to v2, and 1 if v1 is newer than v2.
    """

    # Split the version strings into lists of numbers
    v1_list = [int(x) for x in v1.split(".")]
    v2_list = [int(x) for x in v2.split(".")]

    # If all numbers are equal, the versions are equal
    if v1_list < v2_list:
        return -1
    if v1_list > v2_list:
        return 1
    return 0


class DarwinToDarwinGenerator(ToolchainGenerator):
    """
    Toolchain generator for building on Darwin (macOS) and targeting Darwin platforms.

    Inherits from ToolchainGenerator and provides methods for generating scripts and
    configurations for the C and C++ compilers (cc and cxx).

    Attributes:
        XCODE_VER_RE (str): Regular expression to extract Xcode version.
        XCODE_BUILD_VER (str): Regular expression to extract Xcode build version.
        MIN_VERSION (str): Minimum required Xcode version.
        OSX_DEPLOYMENT_TARGET (str): Minimum targeted macOS version for generated binaries.
        osx_sdk_root (pathlib.Path): Path to the macOS SDK used for building.
    """

    XCODE_VER_RE = re.compile(r"Xcode\s+(\d+\.\d+.\d+)")
    XCODE_BUILD_VER = re.compile(r"Build version\s+(\w+)")
    MIN_VERSION = "11"
    OSX_DEPLOYMENT_TARGET = "11.0"

    def __init__(self, dest, prefix) -> None:
        super().__init__(dest, prefix)
        verinfo = self.run(["xcodebuild", "-version"]).splitlines()

        version = self.XCODE_VER_RE.match(verinfo[0])[1]
        build = self.XCODE_BUILD_VER.match(verinfo[1])[1]

        if compare_versions(version, self.MIN_VERSION) == -1:
            raise ValueError(f"You need at least XCode 10, not {version}")

        xcode_path = self.run(["xcode-select", "-print-path"]).strip()
        sdks = self.parse_xcode_sdks()

        for ver, _ in sdks.get("macOS SDKs", {}).items():
            if compare_versions(ver, self.MIN_VERSION) >= 0:
                self.osx_sdk_root = Path(
                    f"{xcode_path}/Platforms/MacOSX.platform/Developer/SDKs/MacOSX{ver}.sdk"
                )

        logging.info("OSX: Using Xcode: %s (%s)", version, build)
        logging.info("OSX: XCode path: %s", self.osx_sdk_root)

    def _base_script(self):
        script = (
            f"{self.clang()}/bin/clang "
            "-target arm64-apple-darwin23.1.0 "
            f"-isysroot {self.osx_sdk_root} "
            f"-mmacosx-version-min={self.OSX_DEPLOYMENT_TARGET} "
            "-B/usr/bin "
        )
        return script

    def cc(self):
        script = self._base_script()
        extra = "-Wno-unused-command-line-argument"
        return script, extra

    def cxx(self):
        script = self._base_script() + "-stdlib=libc++"
        extra = "-Wno-unused-command-line-argument"
        return script, extra

    def parse_xcode_sdks(self):
        """
        Parse the output of 'xcodebuild -showsdks' and return a dictionary of installed SDKs.

        Returns:
            dict: A dictionary containing information about installed SDKs. The dictionary
            is organized by SDK types, and each SDK type contains a mapping of version numbers
            to their corresponding SDK names.
        """

        output = self.run(["xcodebuild", "-showsdks"])
        sdk_dict = {}
        current_sdk_type = None

        # Split the output into lines and iterate through each line
        lines = output.split("\n")
        for line in lines:
            line = line.strip()
            if not line:
                continue

            # Check for SDK type lines
            # i.e `macOS SDKs:`
            sdk_type_match = re.match(r"^([a-zA-Z ]+):$", line)
            if sdk_type_match:
                current_sdk_type = sdk_type_match.group(1).strip()
                continue

            # Check for SDK details lines
            # i.e. `macOS 14.0                    	-sdk macosx14.0`
            sdk_match = re.match(r"^([a-zA-Z]+ [\d.]+)\s+-sdk (\S+)$", line)
            if sdk_match and current_sdk_type:
                # Version
                sdk_version = sdk_match.group(1).split()[1]
                # Sdk flag.
                sdk_name = sdk_match.group(2)
                sdk_dict.setdefault(current_sdk_type, {})[sdk_version] = sdk_name

        return sdk_dict
