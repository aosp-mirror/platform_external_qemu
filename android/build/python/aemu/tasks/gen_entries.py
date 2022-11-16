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
from pathlib import Path

from aemu.process.command import Command
from aemu.tasks.build_task import BuildTask


class GenEntriesTestTask(BuildTask):
    """The gen entries unit tests."""

    def __init__(self, aosp: Path, destination: Path):
        super().__init__()
        self.aosp = Path(aosp)
        self.destination = Path(destination)

    def do_run(self):
        modes = [
            "def",
            "sym",
            "wrapper",
            "symbols",
            "_symbols",
            "functions",
            "funcargs",
        ]
        search_dir = (
            self.aosp
            / "external"
            / "qemu"
            / "android"
            / "scripts"
            / "tests"
            / "gen-entries"
        )
        script = (
            self.aosp / "external" / "qemu" / "android" / "scripts" / "gen-entries.py"
        )

        for test_dir in search_dir.glob("t.*"):
            (self.destination / test_dir.name).mkdir(parents=True, exist_ok=True)
            for entry in test_dir.glob("*.entries"):
                logging.info("Processing %s", entry)
                for mode in modes:
                    dest = self.destination / entry.with_suffix(f".{mode}").name
                    source = entry.with_suffix(f".{mode}")
                    Command(
                        [
                            sys.executable,
                            script,
                            f"--mode={mode}",
                            entry.absolute(),
                            f"--output={dest}",
                        ]
                    ).run()

                    with open(dest, "r", encoding="utf-8") as dest_file:
                        with open(source, "r", encoding="utf-8") as source_file:
                            src = source_file.read()
                            dst = dest_file.read()
                            assert (
                                src.splitlines()[1:] == dst.splitlines()[1:]
                            ), f"Differences between actual and expected output, see diff -burN {dest} {source}"
