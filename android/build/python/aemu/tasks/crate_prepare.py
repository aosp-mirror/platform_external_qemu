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
from pathlib import Path

from aemu.tasks.build_task import BuildTask


class CratePrepareTask(BuildTask):
    """Create all the symlinks for rust crates."""

    def __init__(self, aosp: Path):
        super().__init__()
        self.aosp = Path(aosp)

    def do_run(self):
        crates = (
            self.aosp
            / "external"
            / "qemu"
            / "android"
            / "third_party"
            / "rust"
            / "crates"
        )
        chromium = crates.parents[4] / "rust" / "chromium" / "crates" / "vendor"

        assert (
            crates.exists()
        ), f"Cannot find {crates} with rust crates."
        assert chromium.exists(), "Cannot find chromium crates, is your repo synced?"

        with open(crates / "symlinks", "r", encoding="utf-8") as links:
            for fname in links.readlines():
                fname = fname.strip()
                source_crate = (chromium / fname).absolute()
                assert (
                    source_crate.exists()
                ), f"The source crate {source_crate} does not exist"
                crate = crates / fname
                crate.unlink(missing_ok=True)
                crate.symlink_to(source_crate, target_is_directory=True)
                logging.debug("Created link from %s -> %s", crate, source_crate)
