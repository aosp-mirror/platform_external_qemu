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


class BuildTask:
    def __init__(self):
        self.enabled = True
        self.name = self.__class__.__name__[:-4]

    def enable(self, enable: bool):
        self.enabled = enable
        return self

    def run(self) -> None:
        """Runs the task if it is enabled."""
        if self.enabled:
            logging.info("Running %s: %s", self.name, self.__class__.__doc__)
            self.do_run()
        else:
            logging.info("Skipping %s: %s", self.name, self.__class__.__doc__)

    def do_run(self) -> None:
        """Subclasses should implement the concrete task."""
