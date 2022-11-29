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
import subprocess
import threading


class LogHandler:
    """A handler that logs lines from a process."""

    def __init__(self, std_out_logger=None, std_err_logger=None):
        self.std_out_log = std_out_logger or logging.info
        self.std_err_log = std_err_logger or logging.error

    def with_std_out_logger(self, log_transform):
        self.std_out_log = log_transform
        return self

    def with_std_err_logger(self, log_transform):
        self.std_err_log = log_transform
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

    def start_log_proc(self, proc: subprocess.Popen):
        """Logs the output of the process in the background.

        stdout will be logged to info level.
        stderr will be logged to error level.

        Args:
            proc (subprocess.Popen): The process to observe.
        """

        for args in [[proc.stdout, self.std_out_log], [proc.stderr, self.std_err_log]]:
            threading.Thread(target=self._reader, args=args).start()
