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
import shutil
import sys
from pathlib import Path

from aemu.process.command import Command
from aemu.tasks.build_task import BuildTask


class EmulatorDistributionNotFoundException(Exception):
    pass


class IntegrationTestTask(BuildTask):
    """Runs the e2e integration tests."""

    def __init__(self, aosp: Path, build_directory: Path, distribution_directory: Path):
        super().__init__()
        self.aosp = Path(aosp)
        self.build_directory = Path(build_directory)
        self.logdir = Path(distribution_directory or build_directory) / "testlogs"

    def do_run(self):
        emulator_dir = self.build_directory / "distribution" / "emulator"
        if shutil.which("emulator", path=emulator_dir) is None:
            raise EmulatorDistributionNotFoundException(
                f"No emulator found in {emulator_dir}, did you run ninja install?"
            )

        launcher = (
            self.aosp
            / "external"
            / "adt-infra"
            / "pytest"
            / "test_embedded"
            / "run_tests.py"
        )
        Command(
            [
                sys.executable,
                launcher,
                "--emulator",
                emulator_dir / "emulator",
                "--session_dir",
                self.build_directory / "session",
                "--logdir",
                self.logdir,
            ]
        ).run()
