#!/usr/bin/env python
#
# Copyright 2020 - The Android Open Source Project
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
import re
import sys
import logging
import absl

class LevelSplitter(logging.StreamHandler):
    """A logging handler that logs everything above the info level
       to stderr.
    """

    def __init__(self):
        super(LevelSplitter, self).__init__(sys.stdout)

    def _log_to_stderr(self, record):
        """Emits the record to stderr.

        This temporarily sets the handler stream to stderr, calls
        StreamHandler.emit, then reverts the stream back.

        Args:
          record: logging.LogRecord, the record to log.
        """
        # emit() is protected by a lock in logging.Handler, so we don't need to
        # protect here again.
        old_stream = self.stream
        self.stream = sys.stderr
        try:
            super(LevelSplitter, self).emit(record)
        finally:
            self.stream = old_stream

    def emit(self, record):
        if record.levelno > 20:
            self._log_to_stderr(record)
        else:
            super(LevelSplitter, self).emit(record)


class NinjaLevelSplitter(LevelSplitter):
    """A logging handler that logs non ninja message to error level.

        This makes sure that cmake configuration and
        build errors are logged atg the error level

        use:

        with NinjaLevelSplitter() as:
            run cmake
    """

    NINJA_LINE = re.compile(r"^\[\d+/\d+\] ")
    CMAKE_LINE = re.compile(r"^-- ")

    def __init__(self):
        self._prev_handler = absl.logging.get_absl_handler()._python_handler
        super(NinjaLevelSplitter, self).__init__()

    def emit(self, record):
        msg = record.msg
        if not NinjaLevelSplitter.NINJA_LINE.match(msg) and not NinjaLevelSplitter.CMAKE_LINE.match(msg):
            record.levelno = 40
            record.levelname = "ERROR"
        super(NinjaLevelSplitter, self).emit(record)

    def __enter__(self):
        self._prev_handler = absl.logging.get_absl_handler()._python_handler
        absl.logging.get_absl_handler()._python_handler = self
        absl.logging.get_absl_handler().activate_python_handler()
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        absl.logging.get_absl_handler()._python_handler = self._prev_handler
