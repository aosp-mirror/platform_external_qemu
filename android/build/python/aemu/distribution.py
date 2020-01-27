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

from __future__ import absolute_import, division, print_function

import os
import re
import shutil
import zipfile

from collections import defaultdict, namedtuple
from absl import logging

from aemu.definitions import get_qemu_root

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
    },
}


def recursive_glob(dir, regex):
    """Recursively glob the rootdir for any file that matches the given regex"""
    reg = re.compile(regex)
    for root, _, filenames in os.walk(dir):
        for filename in filenames:
            fname = os.path.join(root, filename)
            if reg.match(fname):
                yield fname


class License(object):
    """Constructs all the licensing info, by reading all the licensing information of our executables."""

    LicensInfo = namedtuple(
        "LicenseInfo", ["source", "spdx", "license", "local", "exe"]
    )

    def __init__(self, notices, dependencies, installed_targets):
        self.public_libs, self.internal_libs = self._parse_notices(notices)
        self.deps, self.dep_type = self._parse_dependencies(dependencies)
        self.targets = self._parse_installed_targets(installed_targets)
        self.unlicensed = set()
        self.license_closure = {}

        self.lib_to_exe = defaultdict(set)
        for lib in self.public_libs:
            self.lib_to_exe[lib] = set(self.public_libs[lib].exe)

        # target -> licenses
        for dep in self.deps:
            if dep in self.targets and dep != dep.upper():
                for lib in self._license_closure(dep, set()):
                    self.lib_to_exe[lib].add(self.targets[dep])

    def _parse_notices(self, notice):
        public_libs = {}
        internal_libs = {}
        with open(notice, "r") as nfile:
            for line in nfile.readlines():
                # ssl;crypto|boringssl|https://android.googlesource.com/platform/external/boringssl/+/refs/heads/master|OpenSSL|NOTICE|/Users/jansene/src/emu/external/qemu/../boringssl//NOTICE|
                libs, name, source, spdx, remote, local, exe = line.split("|")
                if name.strip() == "":
                    continue

                for lib in libs.split(";"):
                    internal_libs[lib.strip()] = name
                public_libs[name] = License.LicensInfo(
                    source.strip(),
                    spdx.strip(),
                    remote.strip(),
                    local.strip(),
                    exe.strip().split(";"),
                )

        return public_libs, internal_libs

    def _parse_dependencies(self, deps):
        dependencies = {}
        dependency_type = {}
        with open(deps, "r") as nfile:
            for line in nfile.readlines():
                dep, typ, liblist = line.split("|")
                dependencies[dep] = liblist.split(";")
                dependency_type[dep] = typ
        return dependencies, dependency_type

    def _parse_installed_targets(self, targetfile):
        targets = {}
        with open(targetfile, "r") as nfile:
            for line in nfile.readlines():
                tgt, name = line.split(",")
                targets[tgt.strip()] = name.strip()
        return targets

    def _license_closure(self, target, seen):
        """Calculate the license closure of a single target."""
        seen.add(target)
        licenses = set()

        if target in self.internal_libs:
            libname = self.internal_libs[target]
            licenses.add(libname)
        else:
            self.unlicensed.add(target)

        if target not in self.deps:
            return licenses

        for dep in self.deps[target]:
            if dep not in seen:
                licenses = licenses.union(self._license_closure(dep, seen))
        return licenses

    def to_csv(self):
        """Generates a csv file that can be used to export docker files."""
        csv = "External Library Name, Link to License, License Name, Binaries using the license.\n"
        for lib in self.lib_to_exe:
            lic = self.public_libs[lib]
            csv += "{},{},{},{}\n".format(
                lib, lic.license, lic.spdx, " ".join(self.lib_to_exe[lib])
            )
        return csv

    def notice_file(self):
        """Constructs the notices text."""
        notice = ""
        for lib in self.lib_to_exe:
            fnames = self.lib_to_exe[lib]
            notice += "===========================================================\n"
            notice += "Notices for file(s):\n"
            notice += "\n".join(fnames)
            notice += "\n"
            notice += "The notices is included for the library: {}\n\n".format(lib)
            notice += "===========================================================\n"
            with open(self.public_libs[lib].local) as license:
                notice += license.read()
            notice += "\n\n"
        return notice


def create_distribution(dist_dir, build_dir, data):
    """Creates a release by copying files needed for the given platform."""
    if not os.path.exists(dist_dir):
        os.makedirs(dist_dir)

    licensing = License(
        os.path.join(build_dir, "LICENSES.LST"),
        os.path.join(build_dir, "TARGET_DEPS.LST"),
        os.path.join(build_dir, "INSTALL_TARGETS.LST"),
    )

    # Let's find the set of
    zip_set = zip_sets[data["config"]]

    # First we create the individual zip sets using the regular expressions.
    for zip_fname, params in zip_set.iteritems():
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

