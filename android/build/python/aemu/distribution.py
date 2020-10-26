#!/usr/bin/env python
#
# Copyright 2019 - The Android Open Source Project
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


import os
import re
import zipfile
import logging

from aemu.definitions import get_qemu_root
from aemu.licensing import Licensing

# These are the set of zip files that will be generated in the distribution directory.
# You can add additional zip sets here if you need. A file will be added to the given zipfile
# if a file under the --out directory matches the given regular expression.

# For naming you can use the {key} pattern, which will replace the given {key} with the value.
# Most of the absl FLAGS are available.
zip_sets = {
    "debug": {  # Release type..
        # Look for *.gcda files under the build tree
        "code-coverage-{target}.zip": [("{build_dir}", r".*(gcda|gcno)", "/")],
        # Look for all files under {out}/distribution
        "sdk-repo-{target}-emulator-full-debug-{sdk_build_number}.zip": [
            ("{build_dir}/distribution", r".*", "/")
        ],
    },
    "release": {
        "sdk-repo-{target}-debug-emulator-{sdk_build_number}.zip": [
            # Look for all files under {out}/build/debug_info/
            ("{build_dir}/build/debug_info", r".*", "/"),
            ("{build_dir}/lib64", r".*", "/tests/lib64/"),
            ("{build_dir}/lib", r".*", "/tests/lib/"),
            ("{build_dir}/testdata", r".*", "/tests/testdata/"),
        ],
        "sdk-repo-{target}-grpc-samples-{sdk_build_number}.zip": [
            ("{src_dir}/android/android-grpc/docs/grpc-samples/", r".*", "/")
        ],
        # Look for all files under {out}/distribution
        "sdk-repo-{target}-emulator-{sdk_build_number}.zip": [
            ("{build_dir}/distribution", r".*", "/")
        ],
        # crosvm/gfxstream build
        "sdk-repo-{target}-crosvm-host-package-x86_64-{sdk_build_number}.zip": [
            ("{build_dir}/distribution/cf-host-package/x86_64-linux-gnu", r".*", "/")
        ],
        "sdk-repo-{target}-crosvm-host-package-aarch64-{sdk_build_number}.zip": [
            ("{build_dir}/distribution/cf-host-package/aarch64-linux-gnu", r".*", "/")
        ],
    },
}


def recursive_glob(directory, regex):
    """Recursively glob the rootdir for any file that matches the given regex"""
    reg = re.compile(regex)
    for root, _, filenames in os.walk(directory):
        for filename in filenames:
            fname = os.path.join(root, filename)
            if reg.match(fname):
                yield fname


def create_distribution(dist_dir, build_dir, data):
    """Creates a release by copying files needed for the given platform."""
    if not os.path.exists(dist_dir):
        os.makedirs(dist_dir)

    licensing = Licensing(build_dir, get_qemu_root())

    # Let's find the set of
    zip_set = zip_sets[data["config"]]

    # First we create the individual zip sets using the regular expressions.
    for zip_fname, params in zip_set.items():
        zip_fname = os.path.join(dist_dir, zip_fname.format(**data))
        logging.info("Creating %s", zip_fname)

        files = []
        with zipfile.ZipFile(
            zip_fname, "w", zipfile.ZIP_DEFLATED, allowZip64=True
        ) as zipf:
            # Include the notice files
            zipf.writestr("emulator/NOTICE.txt", licensing.notice_file())
            zipf.writestr("emulator/NOTICE.csv", licensing.to_csv())

            for param in params:
                start_dir, regex, dest = param
                search_dir = os.path.normpath(
                    start_dir.format(build_dir=build_dir, src_dir=get_qemu_root())
                )
                for fname in recursive_glob(search_dir, regex.format(data)):
                    arcname = os.path.relpath(fname[len(search_dir) :], "/")
                    arcname = os.path.join(dest, arcname)
                    files.append(arcname)
                    zipf.write(fname, arcname)
