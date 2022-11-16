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

import platform
from distutils.spawn import find_executable
from pathlib import Path

from aemu.platform.toolchains import Toolchain
from aemu.process.command import Command
from aemu.process.environment import get_default_environment
from aemu.tasks.build_task import BuildTask


class CompileTask(BuildTask):
    """Compiles the code base."""

    def __init__(
        self,
        aosp: Path,
        destination: Path,
    ):
        super().__init__()
        # Stripping is meaning less in windows world
        target = "install"
        if platform.system() != "Windows":
            target += "/strip"

        cmake_dir = (
            aosp / "prebuilts" / "cmake" / f"{platform.system().lower()}-x86" / "bin"
        )
        self.cmake_cmd = [
            find_executable("cmake", path=str(cmake_dir)),
            "--build",
            destination,
            "--target",
            target,
        ]
        self.env = get_default_environment(aosp)

    def do_run(self):
        Command(self.cmake_cmd).with_environment(self.env).run()
