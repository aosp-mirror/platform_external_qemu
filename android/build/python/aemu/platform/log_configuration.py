#!/usr/bin/env python3
#
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
import sys


class LogBelowLevel(logging.Filter):
    """A logging filter that only logs a line if it is below the given level."""

    def __init__(self, exclusive_maximum, name=""):
        super(LogBelowLevel, self).__init__(name)
        self.max_level = exclusive_maximum

    def filter(self, record):
        return True if record.levelno < self.max_level else False


def configure_logging(logging_level):
    """Configures the logging system to log at the given level

    Args:
        logging_level (_type_): A logging level, or number.
    """
    logging_handler_out = logging.StreamHandler(sys.stdout)
    logging_handler_out.setLevel(logging.DEBUG)
    logging_handler_out.addFilter(LogBelowLevel(logging.WARNING))

    logging_handler_err = logging.StreamHandler(sys.stderr)
    logging_handler_err.setLevel(logging.WARNING)

    logging.root = logging.getLogger("root")
    logging.root.setLevel(logging_level)
    logging.root.addHandler(logging_handler_out)
    logging.root.addHandler(logging_handler_err)
