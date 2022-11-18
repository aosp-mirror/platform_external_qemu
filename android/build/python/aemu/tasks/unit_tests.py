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
import platform
import shutil
import tempfile
from pathlib import Path

from aemu.process.command import Command
from aemu.tasks.build_task import BuildTask

class AccelerationCheckTask(BuildTask):
    """Verify that we can determine the hypervisor."""

    def __init__(self, destination: Path):
        super().__init__()
        self.destination = Path(destination)

    def do_run(self):
        Command([self.destination / "emulator-check", "accel"]).run()


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

            Command(
                [
                    ctest,
                    "-j",
                    self.jobs,
                    "--output-junit",
                    junit_file.absolute(),
                    "--output-on-failure",
                    "--repeat",
                    "until-pass:2",  # Flaky tests get 2 chances
                    "--timeout",  # Every test gets at most 3 minutes.
                    "180",
                ]
            ).in_directory(self.destination).with_environment(env).run()
