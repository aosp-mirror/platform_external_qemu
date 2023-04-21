# Copyright 2022 - The Android Open Source Project
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
import platform
import re
from pathlib import Path


class Toolchain:

    CPU_ARCHITECTURE_MAP = {
        # Arm based
        "aarch64": "aarch64",
        "aarch64_be": "aarch64",
        "arm64": "aarch64",
        "armv8b": "aarch64",
        "armv8l": "aarch64",
        # Intel based.
        "amd64": "x86_64",
        "i686": "x86_64",
        "x86_64": "x86_64",
    }

    AVAILABLE = {
        "windows-x86_64": "toolchain-windows_msvc-x86_64.cmake",
        "linux-x86_64": "toolchain-linux-x86_64.cmake",
        "darwin-x86_64": "toolchain-darwin-x86_64.cmake",
        "linux-aarch64": "toolchain-linux-aarch64.cmake",
        "darwin-aarch64": "toolchain-darwin-aarch64.cmake",
    }

    DISTRIBUTION = {
        "windows-x86_64": "windows",
        "linux-x86_64": "linux",
        "darwin-x86_64": "darwin",
        "linux-aarch64": "linux_aarch64",
        "darwin-aarch64": "darwin_aarch64",
    }

    def __init__(self, aosp: Path, target: str):
        self.target = self._infer_target(target)
        self.aosp = Path(aosp)

    def _infer_target(self, target):
        """Infers the full target name  given the potential partial target name.

        A complete target name looks like: ${OS}_{$AARCH} where
        OS = [windows, linux, darwin] and AARCH = [aarch64, x86_64]


        Args:
            target (str): The desired target you wish to compile for

        Raises:
            Exception: If the given target cannot be inferred or is not supported.

        Returns:
            str: The actual complete target name.

        """
        if target is None:
            target = platform.system()

        target = target.lower().strip()
        host = platform.system().lower()
        match = re.match("(windows|linux|darwin)([-_](aarch64|x86_64))?", target)

        if match:
            if not match.group(3):
                # Let's infer the architecture
                aarch = platform.machine()

                # Note, this is far from complete, and is best effort, if this does
                # not work you should specify the architecture yourself as the
                # warning is pointing out.
                if aarch == "arm64":
                    aarch = "aarch64"
                if aarch != "aarch64":
                    aarch = "x86_64"

                if host != target:
                    logging.warning(
                        "\n\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="
                    )
                    logging.warning(
                        "--- WARNNG ---  You are cross compiling on %s with target %s and did not specify a machine architecture. Assuming: %s",
                        host,
                        target,
                        aarch,
                    )
                    logging.warning(
                        "--- WARNNG ---  If this does not work you should really specify the desired machine architecture"
                    )
                    logging.warning(
                        "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n"
                    )

                target = target + "-" + aarch
            else:
                target = match.group(1) + "-" + match.group(3)

        if target not in self.AVAILABLE:
            raise Exception(f"Target {target} is not supported.")

        return target

    def distribution(self) -> str:
        return self.DISTRIBUTION[self.target]

    def cmake_toolchain(self) -> str:
        """Returns the path to the cmake toolchain file."""
        return (
            self.aosp
            / "external"
            / "qemu"
            / "android"
            / "build"
            / "cmake"
            / self.AVAILABLE[self.target]
        )

    def is_crosscompile(self) -> bool:
        """True if the given target needs to be cross compiled.

        Args:
            target (str): The complete target.

        Raises:
            Exception: If the given target cannot be inferred or is not supported.


        Returns:
            bool: True if we will crosscompile on this machine for the given target.
        """
        target = self.target.lower().strip()
        match = re.match("(windows|linux|darwin)([-_](aarch64|x86_64))?", target)
        if not (match and match.group(3)):
            raise Exception(f"Malformed target: {target}.")

        aarch = self.CPU_ARCHITECTURE_MAP.get(platform.machine().lower(), "unknown_cpu")
        if aarch == "arm64":
            aarch = "aarch64"
        else:
            # Ok maybe python is running under rosetta, if so the uname.version
            # will have have something along RELEASE_ARM64_T6000 in it
            uname = platform.uname()
            if "ARM64" in uname.version and uname.system == "Darwin":
                aarch = "aarch64"

        return aarch != match.group(3)
