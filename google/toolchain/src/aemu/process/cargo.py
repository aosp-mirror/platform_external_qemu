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
import os
import platform
import shutil
from pathlib import Path

from aemu.process.runner import run
from aemu.toolchains.toolchain_generator import ToolchainGenerator


class Cargo:
    """Class for managing CMake builds and installations within an AOSP context."""

    def __init__(self, aosp: Path, toolchain: ToolchainGenerator, dist: Path):
        """Initializes a CMake object for managing builds.

        Args:
            aosp: Path to the AOSP (Android Open Source Project) root directory.
            dist: Path to the installation distribution directory.
        """
        self.aosp = aosp
        self.toolchain = toolchain
        self.exe = toolchain.dest / "cargo"
        self.dist = dist

    def host(self) -> str:
        """Returns the lowercase name of the host operating system."""
        return platform.system().lower()

    def build_rutabaga_gfx(self, output: Path) -> str:
        ffi = self.aosp / "external" / "crosvm" / "rutabaga_gfx" / "ffi"
        manifest = ffi / "Cargo.toml"

        gfx_env = os.environ.copy()
        gfx_env["GFXSTREAM_PATH"] = str(self.dist)
        gfx_env["RUST_BACKTRACE"] = "full"
        if self.host() == "darwin":
            gfx_env["RUSTFLAGS"] = "-C link-args=-Wl,-rpath,@loader_path"

        run(
            [
                self.exe,
                "build",
                "--features=gfxstream",
                "--verbose",
                "--release",
                f"--manifest-path={manifest}",
                # Sadly this does not work on MacOs..
                # f"--target-dir",
                # output,
            ]
            + self.toolchain.rust_flags(),
            cwd=ffi,
            env=gfx_env,
            toolchain_path=self.toolchain.dest,
        )

        output.mkdir(exist_ok=True, parents=True)
        shutil.move(str(ffi / "target"), output)
        inc_dir: Path = output / "target" / "release" / "include" / "rutabaga_gfx"
        inc_dir.mkdir(parents=True, exist_ok=True)
        inc = (
            self.aosp
            / "external"
            / "crosvm"
            / "rutabaga_gfx"
            / "ffi"
            / "src"
            / "include"
            / "rutabaga_gfx_ffi.h"
        )
        shutil.copy(inc, inc_dir)
        return output / "target" / "release"

    def build_target(self, cmake_target: str) -> str:
        """Configures, builds, and installs a specified CMake target.

        Args:
            cmake_target: The CMake target in the format 'source_path:label'.

        Returns:
            The path to the installed target.
        """
        tgt_idx = cmake_target.index(":")
        label = cmake_target[tgt_idx + 1 :]
        output = self.dist / "cargo" / label
        output.mkdir(parents=True, exist_ok=True)

        # Note we can only build rutabaga at this time..
        if label == "rutabaga_gfx_ffi":
            return self.build_rutabaga_gfx(output)
