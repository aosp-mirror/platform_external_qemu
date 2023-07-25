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
import logging
import os
import platform
from pathlib import Path

from aemu.platform.toolchains import Toolchain
from aemu.tasks.build_task import BuildTask
from aemu.process.py_runner import PyRunner


class EmulatorDistributionNotFoundException(Exception):
    pass


class IntegrationTestTask(BuildTask):
    """Runs the e2e integration tests."""

    def __init__(
        self,
        aosp: Path,
        target: str,
        build_directory: Path,
        distribution_directory: Path,
    ):
        """Initializes an instance of IntegrationTestTask.

        It can run in 2 modes:
            - There is a distribution directory:
                    In this mode it will mimic what the tests on test infrastructure runs
            - There is only a build directory:
                    In this mode it is assumed that you wish to run it against your local build.

        Args:
            aosp (Path): The path to the AOSP root directory.
            target (str): The target, for example linux-aarch64
            build_directory (Path): The path to the build directory.
            distribution_directory (Path): The optional path to the distribution directory.
        """
        super().__init__()
        self.aosp = Path(aosp)
        self.dist = distribution_directory
        self.build_directory = Path(build_directory)
        self.target = target
        self.logdir = Path(distribution_directory or build_directory) / "testlogs"
        self.launcher = (
            self.aosp
            / "external"
            / "adt-infra"
            / "pytest"
            / "test_embedded"
            / "run_tests.py"
        )
        self.build_target = os.environ.get("BUILD_TARGET_NAME", "")
        self.build_target_opt = ["--build_target", self.build_target] if self.build_target else []

    def run_from_dist(self, py):
        """Runs the integration tests from the distribution directory."""
        py.run(
            [
                self.launcher,
                "--build_dir",
                self.dist,
                "--logdir",
                self.logdir,
                "--failures_as_errors",
            ] + self.build_target_opt
        )

    def run_from_build(self, py):
        """Runs the integration tests from the build directory."""

        if Toolchain(self.aosp, self.target).is_crosscompile():
            logging.warning("Cannot run integration tests when cross compiling.")
            return

        emulator_dir = self.build_directory / "distribution" / "emulator"
        if shutil.which("emulator", path=emulator_dir) is None:
            raise EmulatorDistributionNotFoundException(
                f"No emulator found in {emulator_dir}, did you run ninja install?"
            )
        py.run(
            [
                self.launcher,
                "--emulator",
                emulator_dir / "emulator",
                "--symbols",
                self.build_directory / "build" / "symbols",
                "--logdir",
                self.logdir,
                "--failures_as_errors",
            ] + self.build_target_opt
        )

    def do_run(self):
        repo = self.aosp / "external" / "adt-infra" / "devpi" / "repo" / "simple"
        if platform.system() != "Windows":
            repo = f"file://{repo}"
        py_runner = PyRunner(repo, self.aosp)

        if self.dist:
            self.run_from_dist(py_runner)
        else:
            self.run_from_build(py_runner)
