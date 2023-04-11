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
import platform
import shutil
import subprocess
import os
from pathlib import Path

from aemu.process.command import Command
from aemu.tasks.build_task import BuildTask


class FixCargoTask(BuildTask):
    """Tries to fix Rust Cargo errors on Windows, BE CAREFUL, it deletes a resyncs part of your repo."""

    def __init__(self, aosp: str):
        super().__init__()
        self.aosp = Path(aosp)
        if platform.system() == "Windows":
            status = subprocess.check_output(
                ["git", "config", "--get", "core.autocrlf"], encoding="utf-8"
            )
            if status.lower().rstrip() != "false":
                logging.warning(
                    "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"
                )
                logging.warning(
                    "Your git core.autorcrlf settings is not set to false but %s.",
                    status.lower().rstrip(),
                )
                logging.warning(
                    "You likely need to fixup your repo by running %s --task FixCargo",
                    Path("android") / "rebuild.cmd",
                )
                logging.warning(
                    "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"
                )

    def do_run(self):
        logging.info(
            "This tries to fix up build checksum errors, which are likely due to git crlf settings"
        )
        status = subprocess.check_output(["git", "config", "--get", "core.autocrlf"])
        if status.lower().rstrip() == "false":
            logging.warning(
                "============================================================================================"
            )
            logging.warning(
                "Note: your git core.autorcrlf settings is already set to false.. You might not need this.."
            )
            logging.warning(
                "============================================================================================"
            )

        logging.info("This is a destructive operation, you can lose data.")
        logging.info("This will delete parts of your repo, and resync them")
        logging.info("Press ctrl-c to quit now, or press enter to try your luck.")
        input()

        # Known paths where we use rust.
        fix_paths = [
            self.aosp / "packages" / "modules" / "Bluetooth",
            self.aosp / "tools" / "netsim",
            self.aosp / "external" / "qemu",
        ]

        # Fixup the crlf setting
        Command(["git", "config", "--global", "core.autocrlf", "false"]).run()
        for path in fix_paths:
            if os.path.exists(path):
                logging.info("Stashing changes and removing %s", path)
                Command(["git", "stash"]).in_directory(path).run()
                for fname in path.glob("*"):
                    if fname.name == ".git":
                        logging.info("Not removing git directory.")
                    shutil.rmtree(fname, ignore_errors=True)

                logging.info("Bringing back active changes..")
                Command(["git", "checkout", "."]).in_directory(path).run()
                Command(["git", "stash", "pop"]).in_directory(path).ignore_failures().run()
            else:
                logging.warning("Skipping attempt to fix crlf setting on invalid directory: %s", path)

        logging.info(
            "Updated your git settings, and re-synced, you will need to run %s, as we cleared out everything.",
            Path("android") / "rebuild.cmd",
        )
