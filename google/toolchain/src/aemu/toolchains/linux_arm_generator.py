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

from aemu.toolchains.toolchain_generator import ToolchainGenerator
import shutil
import logging


class LinuxToLinuxAarch64Generator(ToolchainGenerator):
    """A cross compilation toolchain configurator using gcc

    It will try to use the hardcoded version if available, otherwise
    it will fallback to the default one.
    """

    GCC_VER = 10
    GCC_PREFIX = "aarch64-linux-gnu-"

    def _tool(self, tool):
        gcc = shutil.which(f"{self.GCC_PREFIX}{tool}")
        return gcc, ""

    def cc(self, compiler="gcc"):
        versioned = f"{self.GCC_PREFIX}{compiler}-{self.GCC_VER}"
        gcc = shutil.which(versioned)
        if not gcc:
            logging.warning(
                "%s not found, Falling back to non versioned compiler", versioned
            )
            gcc, _ = self._tool(compiler)
        cache = f"{self.ccache}" if self.ccache else ""
        script = f"{cache} {gcc} "
        return script, ""

    def cxx(self):
        return self.cc("g++")

    def lld(self):
        ld, _ = self._tool("ld")
        return (
            f"{ld} -L/usr/lib/gcc/aarch64-linux-gnu/5",
            "-lc -lc++ -lm -lgcc -lgcc_s -ldl",
        )

    def link_dirs(self):
        pass

    def nm(self):
        return self._tool("nm")

    def strip(self):
        return self._tool("strip")
