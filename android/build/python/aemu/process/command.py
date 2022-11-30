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
import os
import platform
import subprocess
from pathlib import Path

from aemu.process.environment import BaseEnvironment
from aemu.process.log_handler import LogHandler


class CommandFailedException(Exception):
    pass


class Command:
    """A Command that can be run in a shell."""

    def __init__(self, cmd):
        self.env = os.environ
        self.cmd = [str(c) for c in cmd]
        self.log_handler = LogHandler()
        self.working_directory = None
        self.use_shell = platform.system() == "Windows"

    def with_environment(self, env: BaseEnvironment):
        """Additional environment to use when running this command

        Args:
            env (dict[str, str]): Environment used to override
                                default os environment.

        Returns:
            Command: The command itself
        """
        self.env.update(env)
        return self

    def with_log_handler(self, handler: LogHandler):
        """Sets the log

        Args:
            handler (LogHandler): The loghandler responsible for logging

        Returns:
            Command: The command itself
        """
        self.log_handler = handler
        return self

    def in_directory(self, directory):
        self.working_directory = Path(directory)
        return self

    def run(self):
        """Runs the given command.

        Raises:
            CommandFailedException: Raised if the status code is non zero
        """
        cmdstr = " ".join(self.cmd)
        logging.info("Run: %s in %s", cmdstr, self.working_directory)

        proc: subprocess.Popen = subprocess.Popen(
            self.cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=self.working_directory,
            shell=self.use_shell,
            env=self.env,
            encoding="utf-8",
        )

        self.log_handler.start_log_proc(proc)
        if proc.wait() != 0:
            raise CommandFailedException(f"{cmdstr} Status: {proc.returncode} != 0")
