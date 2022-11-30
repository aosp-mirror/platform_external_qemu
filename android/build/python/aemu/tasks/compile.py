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
import logging
import re
import shutil
from pathlib import Path

from aemu.process.command import Command
from aemu.process.log_handler import LogHandler
from aemu.process.environment import get_default_environment
from aemu.tasks.build_task import BuildTask


class CompileTask(BuildTask):
    """Compiles the code base."""

    NINJA_FILTER = re.compile(r"^\[\d+/\d+\]")

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
            shutil.which("cmake", path=str(cmake_dir)),
            "--build",
            destination,
            "--target",
            target,
        ]
        self.env = get_default_environment(aosp)

    def filter_ninja_error(self, logline: str):
        """Filters ninja lines to std out, and failures to stderr

        Args:
            logline (str): Logline that is to be filtered.
        """
        if self.NINJA_FILTER.match(logline):
            logging.info(logline)
        else:
            logging.error(logline)

    def do_run(self):
        ninja_err_filter = LogHandler(self.filter_ninja_error)
        Command(self.cmake_cmd).with_environment(self.env).with_log_handler(
            ninja_err_filter
        ).run()
