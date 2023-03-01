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
import re
import shutil
import zipfile
from pathlib import Path

from aemu.platform.toolchains import Toolchain
from aemu.tasks.build_task import BuildTask


class InvalidTargetException(Exception):
    pass


class DistributionTask(BuildTask):
    """Create an emulator distribution."""

    # These are the set of zip files that will be generated in the distribution directory.
    # You can add additional zip sets here if you need. A file will be added to the given zipfile
    # if a file under the --out directory matches the given regular expression.

    # For naming you can use the {key} pattern, which will replace the given {key} with the value.
    # Most of the absl FLAGS are available.
    zip_sets = {
        "debug": {  # Release type..
            # Look for *.gcda files under the build tree
            "code-coverage-{target}.zip": [("{build_dir}", r".*(gcda|gcno)")],
            # Look for all files under {out}/distribution
            "sdk-repo-{target}-emulator-full-debug-{sdk_build_number}.zip": [
                ("{build_dir}/distribution", r".*")
            ],
        },
        "release": {
            "sdk-repo-{target}-debug-emulator-{sdk_build_number}.zip": [
                # Look for all files under {out}/build/debug_info/
                ("{build_dir}/build/debug_info", r".*"),
            ],
            "sdk-repo-{target}-emulator-symbols-{sdk_build_number}.zip": [
                # Look for all files under {out}/build/debug_info/ with the .sym extension
                ("{build_dir}/build/symbols", r".*sym"),
            ],
            # Look for all files under {out}/distribution
            "sdk-repo-{target}-emulator-{sdk_build_number}.zip": [
                ("{build_dir}/distribution", r".*")
            ],
        },
    }

    TARGET_MAP = {
        "linux-x86-64": "linux",
    }

    def __init__(
        self,
        aosp: Path,
        build_directory: Path,
        distribution_directory: Path,
        target: str,
        sdk_build_number: str,
        configuration: str,
    ):
        super().__init__()
        valid_targets = [
            "linux",
            "linux_aarch64",
            "darwin_aarch64",
            "darwin",
            "windows",
        ]
        if not target in valid_targets:
            raise InvalidTargetException(
                f"Unknown distribution target! {target} not in {valid_targets}."
            )

        self.build_dir = Path(build_directory)
        self.dist_dir = Path(distribution_directory) if distribution_directory else None
        self.src_dir = Path(aosp) / "external" / "qemu"
        self.data = {
            "aosp": str(aosp),
            "target": target,
            "sdk_build_number": sdk_build_number,
            "config": configuration,
        }

    def do_run(self):
        if not self.dist_dir:
            logging.warning("No distribution directory.. skipping")
            return

        self.dist_dir.mkdir(exist_ok=True, parents=True)

        src_cov_name = Path(self.build_dir) / "lcov"
        if src_cov_name.is_file():
            dist_cov_dir = self.dist_dir / "coverage"
            dist_cov_dir.mkdir(exist_ok=True, parents=True)
            dist_cov_name = dist_cov_dir / "lcov"
            logging.info("Copying %s ==> %s", src_cov_name, dist_cov_name)
            shutil.copy(src_cov_name, dist_cov_name)

        # Let's find the set of
        zip_set = self.zip_sets[self.data["config"]]

        # First we create the individual zip sets using the regular expressions.
        for zip_fname, params in list(zip_set.items()):
            zip_fname = self.dist_dir / zip_fname.format(**self.data)
            logging.info("Creating %s", zip_fname)

            with zipfile.ZipFile(
                zip_fname, "w", zipfile.ZIP_DEFLATED, allowZip64=True
            ) as zipf:
                for start_dir, regex in params:
                    file_matcher_regex = re.compile(regex.format(self.data))
                    search_dir = Path(
                        start_dir.format(build_dir=self.build_dir, src_dir=self.src_dir)
                    )
                    logging.debug("Searching %s", search_dir)
                    for fname in search_dir.glob("**/*"):
                        logging.debug("Checking %s with %s", fname, file_matcher_regex)
                        if not file_matcher_regex.match(str(fname)):
                            continue

                        arcname = fname.relative_to(search_dir)
                        logging.debug("Adding %s as %s", fname, arcname)
                        zipf.write(fname, arcname)
