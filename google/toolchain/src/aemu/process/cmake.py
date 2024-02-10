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
import platform
import logging
from pathlib import Path

from aemu.process.runner import run


class CMake:
    """Class for managing CMake builds and installations within an AOSP context."""

    def __init__(self, aosp: Path, toolchain: Path, dist: Path):
        """Initializes a CMake object for managing builds.

        Args:
            aosp: Path to the AOSP (Android Open Source Project) root directory.
            dist: Path to the installation distribution directory.
        """
        self.aosp = aosp
        self.toolchain = toolchain
        self.exe = toolchain / "cmake"
        self.dist = dist

    def host(self) -> str:
        """Returns the lowercase name of the host operating system."""
        return platform.system().lower()

    def build_target(self, cmake_target: str) -> str:
        """Configures, builds, and installs a specified CMake target.

        Args:
            cmake_target: The CMake target in the format 'source_path:label'.

        Returns:
            The path to the installed target.
        """
        tgt_idx = cmake_target.index(":")
        label = cmake_target[tgt_idx + 1 :]
        source_path = self.aosp / Path(cmake_target[:tgt_idx]).relative_to("/")
        output = self.dist / "cmake" / label
        output.mkdir(parents=True, exist_ok=True)
        ext = ".cmd" if self.host() == "windows" else ""
        ninja = (self.toolchain / f"ninja{ext}").as_posix()
        gcc = (self.toolchain / f"gcc{ext}").as_posix()
        gxx = (self.toolchain / f"g++{ext}").as_posix()
        ar = (self.toolchain / f"ar{ext}").as_posix()

        # First we configure the cmake target..
        run(
            [
                self.exe,
                "-S",
                source_path,
                "-B",
                output,
                "-D",
                f"CMAKE_AR={ar}",
                "-D",
                f"CMAKE_MAKE_PROGRAM={ninja}",
                "-D",
                f"CMAKE_C_COMPILER={gcc}",
                "-D",
                f"CMAKE_CXX_COMPILER={gxx}",
                "-G Ninja",
            ],
            toolchain_path=self.toolchain,
        )

        # Next we build it
        run(
            [
                self.exe,
                "--build",
                output,
                "--config",
                "Release",
            ]
        )
        return output
