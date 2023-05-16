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
import glob
import logging
import platform
import shutil
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

from aemu.process.command import Command, CommandFailedException
from aemu.process.log_handler import LogHandler
from aemu.tasks.build_task import BuildTask


class AccelerationCheckTask(BuildTask):
    """Verify that we can determine the hypervisor."""

    def __init__(self, destination: Path):
        super().__init__()
        self.destination = Path(destination)

    def do_run(self):
        Command(
            [self.destination / "distribution" / "emulator" / "emulator-check", "accel"]
        ).run()


class CTestTask(BuildTask):
    """Runs all the unit tests with ctest"""

    def __init__(
        self,
        aosp: Path,
        destination: Path,
        concurrency: int,
        with_gfx_stream: bool,
        distribution_directory: Path,
    ):
        super().__init__()
        self.aosp = Path(aosp)
        self.destination = Path(destination)
        if distribution_directory is None:
            self.distribution = self.destination
        else:
            self.distribution = Path(distribution_directory)
        self.jobs = concurrency
        self.gfxstream = with_gfx_stream


    def do_run(self):
        ctest = shutil.which(
            cmd="ctest",
            path=(
                self.aosp
                / "prebuilts"
                / "cmake"
                / f"{platform.system().lower()}-x86"
                / "bin"
            ),
        )

        # Create the testlog in the right place so it will be picked up
        # by our test scraper.
        junit_file = self.distribution / "testlogs" / "test_results.xml"
        junit_file.parent.mkdir(parents=True, exist_ok=True)

        with tempfile.TemporaryDirectory() as tmpdir:
            env = {
                "ANGLE_DEFAULT_PLATFORM": "swiftshader",
                "TEMP": str(tmpdir),
                "TMP": str(tmpdir),
                "TMPDIR": str(tmpdir),
            }

            if self.gfxstream:
                env["VK_ICD_FILENAMES"] = str(
                    self.destination / "lib64" / "vulkan" / "vk_swiftshader_icd.json"
                )
                env["LD_LIBRARY_PATH"] = str(
                    str(self.destination / "lib64" / "vulkan")
                    + ":"
                    + str(self.destination / "lib64" / "gles_swiftshader")
                )
                env["ANDROID_EMU_VK_ICD"] = "swiftshader"

            try:
                Command(
                    [
                        ctest,
                        "-j",
                        self.jobs,
                        "--output-junit",
                        junit_file.absolute(),
                        "--timeout",  # Every test gets at most 3 minutes.
                        "180",
                    ]
                ).in_directory(self.destination).with_environment(env).run()
            except CommandFailedException:
                # Okay we have failures, let's give ourselves a 2nd chance
                # and log all the failures we encounter directly.
                logging.critical(
                    "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="
                )
                Command(
                    [
                        ctest,
                        "-j",
                        self.jobs,
                        "--rerun-failed",
                        "--output-on-failure",
                        "--timeout",  # Every test gets at most 3 minutes.
                        "180",
                    ]
                ).in_directory(self.destination).with_environment(env).run()


class CoverageReportTask(BuildTask):
    """Generates the code coverage report. CTestTask needs to run prior to this."""

    def __init__(
        self,
        aosp: Path,
        destination: Path,
    ):
        super().__init__()
        self.aosp = Path(aosp)
        self.destination = Path(destination)

    def lcov_out(self, logline: str):
        """Writes all output to lcov file

        Args:
            logline (str): Logline that is to be filtered.
        """
        logging.info("****josh: " + logline)

    def do_run(self):
        clang_version = "clang-r487747c"
        clang_path = (
            self.aosp
            / "prebuilts"
            / "clang"
            / "host"
            / f"{platform.system().lower()}-x86"
            / clang_version
            / "bin"
        )
        llvm_profdata = shutil.which(
            cmd="llvm-profdata",
            path=clang_path,
        )
        llvm_cov = shutil.which(
            cmd="llvm-cov",
            path=clang_path,
        )
        profdata_name = "qemu.profdata"
        # Generate the coverage report if raw profile files were created after
        # running the unittests.
        wc = self.destination / "*.profraw"
        profraws = glob.glob(str(wc))
        if not profraws:
            logging.info("No .profraw files present. Skipping coverage report.")
        else:
            logging.info("Found profraw files. Generating coverage report.")
            try:
                Command(
                    [llvm_profdata, "merge", "-sparse"]
                    + profraws
                    + ["-o", profdata_name]
                ).in_directory(self.destination).run()
            except CommandFailedException:
                logging.critical("llvm_profdata failed")
                raise
            try:
                cov_objs = ["--object=" + i.replace(".profraw", "") for i in profraws]

                with open(self.destination / "lcov", "w") as lcov_file:
                    lcov_out = LogHandler(lambda s: lcov_file.write(s + "\n"))
                    Command(
                        [
                            llvm_cov,
                            "export",
                            "--format",
                            "lcov",
                            "--instr-profile=" + profdata_name,
                        ]
                        + cov_objs
                    ).in_directory(self.destination).with_log_handler(lcov_out).run()
            except CommandFailedException:
                logging.critical("llvm_cov failed")
                raise
