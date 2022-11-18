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
import shutil
from pathlib import Path

from aemu.process.command import Command
from aemu.tasks.build_task import BuildTask


class EmugenTestTask(BuildTask):
    """The gen entries unit tests."""

    def __init__(self, aosp: Path, destination: Path):
        super().__init__()
        self.aosp = Path(aosp)
        self.destination = Path(destination)

    def _create_directory(self, directory: Path) -> Path:
        directory.mkdir(parents=True, exist_ok=True)
        return directory

    def _compare_file(self, source: Path, dest: Path):
        with open(dest, "r", encoding="utf-8") as dest_file:
            with open(source, "r", encoding="utf-8") as source_file:
                src = source_file.read()
                dst = dest_file.read()
                if src.splitlines()[2:] != dst.splitlines()[2:]:
                    logging.error(
                        "FAILURE: Differences between actual and expected output, see diff -burN %s %s",
                        dest,
                        source,
                    )

    def _directory_compare(self, src_directory: Path, dest_directory: Path):
        for src in src_directory.glob("**/*"):
            if src.is_dir():
                continue

            self._compare_file(src, dest_directory / src.relative_to(src_directory))

    def do_run(self):
        logging.debug("Looking for emugen in %s", self.destination)
        emugen = shutil.which("emugen", path=self.destination)
        search_dir = (
            self.aosp
            / "external"
            / "qemu"
            / "android"
            / "android-emugl"
            / "host"
            / "tools"
            / "emugen"
            / "tests"
        )

        for test_dir in search_dir.glob("t.*"):
            output_dir = self.destination / test_dir.name
            encoder = self._create_directory(output_dir / "encoder")
            decoder = self._create_directory(output_dir / "decoder")
            wrapper = self._create_directory(output_dir / "wrapper")
            for entry in test_dir.glob("input/*.in"):
                prefix = entry.relative_to(test_dir / "input").with_suffix("")
                logging.info("Processing %s", entry)
                Command(
                    [
                        emugen,
                        "-i",
                        test_dir / "input",
                        "-D",
                        decoder,
                        "-E",
                        encoder,
                        "-W",
                        wrapper,
                        prefix,
                    ]
                ).run()
                self._directory_compare(test_dir / "expected", output_dir)
