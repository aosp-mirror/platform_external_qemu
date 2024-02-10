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
import shutil
from pathlib import Path

from aemu.process.runner import check_output
from aemu.toolchains.toolchain_generator import ToolchainGenerator


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

    #  Matches Xcode 12.4 as well as Xcode 15.0.1
    XCODE_VER_RE = re.compile(r"Xcode\s+((\d+\.?)+)")
    XCODE_BUILD_VER = re.compile(r"Build version\s+(\w+)")
    MIN_VERSION = "11"
    OSX_DEPLOYMENT_TARGET = "11.0"

    def __init__(self, aosp, dest, prefix, target_arch="arm64") -> None:
        super().__init__(aosp, dest, prefix)
        self.target_arch = target_arch

        verinfo = check_output(["xcodebuild", "-version"]).splitlines()
        version = self.XCODE_VER_RE.match(verinfo[0])[1]
        build = self.XCODE_BUILD_VER.match(verinfo[1])[1]

        if compare_versions(version, self.MIN_VERSION) == -1:
            raise ValueError(f"You need at least XCode 10, not {version}")

        xcode_path = check_output(["xcode-select", "-print-path"]).strip()
        sdks = self.parse_xcode_sdks()

        for ver, _ in sdks.get("macOS SDKs", {}).items():
            if compare_versions(ver, self.MIN_VERSION) >= 0:
                self.osx_sdk_root = Path(
                    f"{xcode_path}/Platforms/MacOSX.platform/Developer/SDKs/MacOSX{ver}.sdk"
                )

        logging.info("OSX: Using Xcode: %s (%s)", version, build)
        logging.info("OSX: XCode path: %s", self.osx_sdk_root)

    def _base_script(self, clang):
        cache = f"{self.ccache}" if self.ccache else ""
        script = (
            f"{cache} {self.clang()}/bin/{clang} "
            f"-arch {self.target_arch} "
            f"-isysroot {self.osx_sdk_root} "
            f"-mmacosx-version-min={self.OSX_DEPLOYMENT_TARGET} "
            "-D__ENVIRONMENT_OS_VERSION_MIN_REQUIRED__=__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ "
            "-B/usr/bin "
        )
        return script

    def strip(self):
        script = "target=$(basename $1)\n"
        script += f"{self.clang() / 'bin' / 'dsymutil'} --out build/debug_info/$target.dSYM $1\n"
        script += "# EXPLICITLY DISABLE ARBITRARY ARGUMENTS"
        return script, ""

    def cc(self):
        script = self._base_script("clang")
        extra = "-Wno-unused-command-line-argument"
        return script, extra

    def cxx(self):
        script = self._base_script("clang++") + " -stdlib=libc++"
        extra = "-Wno-unused-command-line-argument"
        return script, extra

    def gen_toolchain(self):
        super().gen_toolchain()
        self.gen_script("objc", self.dest / f"{self.prefix}objc", self.cc)

    def parse_xcode_sdks(self):
        """
        Runs and parses the output of 'xcodebuild -showsdks' and return a dictionary of installed SDKs.

        Returns:
            dict: A dictionary containing information about installed SDKs. The dictionary
            is organized by SDK types, and each SDK type contains a mapping of version numbers
            to their corresponding SDK names.
        """

        # Sample output of xcodebuild -showsdks

        # DriverKit SDKs:
        # 	DriverKit 23.0                	-sdk driverkit23.0

        # iOS SDKs:
        # 	iOS 17.0                      	-sdk iphoneos17.0

        # macOS SDKs:
        # 	macOS 14.0                    	-sdk macosx14.0
        # 	macOS 14.0                    	-sdk macosx14.0

        # watchOS SDKs:
        # 	watchOS 10.0                  	-sdk watchos10.0

        output = check_output(["xcodebuild", "-showsdks"])
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


class DarwinToDarwinX64Generator(DarwinToDarwinGenerator):
    def __init__(self, aosp, dest, prefix) -> None:
        super().__init__(aosp, dest, prefix, "x64")
