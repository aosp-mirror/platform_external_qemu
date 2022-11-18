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
import threading
from pathlib import Path

from aemu.process.environment import BaseEnvironment


class CommandFailedException(Exception):
    pass


class Command:
    """A Command that can be run in a shell."""

    def __init__(self, cmd):
        self.env = os.environ
        self.cmd = [str(c) for c in cmd]
        self.std_out_transform = lambda x: x
        self.std_err_transform = lambda x: x
        self.working_directory = None
        self.use_shell = platform.system() == "Windows"

    def with_environment(self, env: BaseEnvironment):
        """Additional environment to use when running this command

        Args:
            env (dict[str, str]): Environment used to override
                                default os environment.

        Returns:
            Command: _description_
        """
        self.env.update(env)
        return self

    def with_std_out_transform(self, log_transform):
        self.std_out_transform = log_transform
        return self

    def with_std_err_transform(self, log_transform):
        self.std_err_transform = log_transform
        return self

    def in_directory(self, directory):
        self.working_directory = Path(directory)
        return self

    def _reader(self, pipe, logfn):
        """Log every line from the pipe, by calling
        logn for every line,

        Args:
            pipe (_type_): Pipe with lines in utf-8
            logfn (_type_): Function used to log a line
        """
        try:
            for line in iter(pipe.readline, ""):
                logfn(line[:-1].strip())
        finally:
            pass

    def _log_proc(self, proc: subprocess.Popen):
        """Logs the output of the process in the background.

        stdout will be logged to info level.
        stderr will be logged to error level.

        Args:
            proc (subprocess.Popen): The process to observe.
        """
        info_log = lambda x: logging.info(self.std_out_transform(x))
        error_log = lambda x: logging.error(self.std_err_transform(x))

        for args in [[proc.stdout, info_log], [proc.stderr, error_log]]:
            threading.Thread(target=self._reader, args=args).start()

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

        self._log_proc(proc)
        if proc.wait() != 0:
            raise CommandFailedException(f"{cmdstr} Status: {proc.returncode} != 0")
