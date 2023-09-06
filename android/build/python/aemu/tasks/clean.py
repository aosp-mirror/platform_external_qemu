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
import shutil
from pathlib import Path

from aemu.tasks.build_task import BuildTask


class CleanTask(BuildTask):
    """Deletes the destination directory."""

    def __init__(self, destination: Path, aosp: Path) -> None:
        super().__init__()
        self.destination = Path(destination)
        self.bazel = Path(aosp)

    def do_run(self) -> None:
        if self.destination.exists():
            logging.info("Cleaning %s", self.destination)
            shutil.rmtree(self.destination, ignore_errors=True)
        for bazel in self.bazel.glob("bazel*"):
            logging.info("Cleaning %s", bazel)
            shutil.rmtree(bazel, ignore_errors=True)

        self.destination.mkdir(parents=True, exist_ok=True)