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
from collections import defaultdict
from aemu.definitions import get_qemu_root

class License(object):
    def __init__(self, name, source_url, spdx, license_url, local_license):
        self.name = name
        self.source_url = source_url
        self.spdx = spdx
        self.license_url = license_url
        self.internal = "INTERNAL" in local_license
        self.local_license = local_license.replace("INTERNAL", get_qemu_root())
        self.exes = set()
        self.text = None

    def is_internal(self):
        return self.internal or self.spdx == "None"

    def register_exes(self, exes):
        self.exes = self.exes.union(exes)

    def read(self):
        if self.text:
            return self.text
        if not os.path.exists(self.local_license):
            print(self.name, self.spdx)
        with open(self.local_license, "r") as lfile:
            self.text = lfile.read()
        return self.text

    def to_csv(self):
        return "{}, {}, {}, {}, {}".format(
            self.name, self.license_url, self.spdx, " ".join(self.exes), self.source_url
        )

    def __str__(self):
        return self.name


class Licensing(object):
    """Constructs all the licensing info, by reading all the licensing information of our executables."""

    # These are the sets of libraries that basically come from the local system.
    common_local_libs = [
        "m",
        "dl",
        "pthread",
        "Threads::Threads",
        "rt",
        "atls::atls",
        "ws2_32::ws2_32",
        "d3d9::d3d9",
        "ole32::ole32",
        "psapi::psapi",
        "iphlpapi::iphlpapi",
        "wldap32::wldap32",
        "shell32::shell32",
        "user32::user32",
        "advapi32::advapi32",
        "secur32::secur32",
        "mfuuid::mfuuid",
        "winmm::winmm",
        "shlwapi::shlwapi",
        "gdi32::gdi32",
        "dxguid::dxguid",
        "wininet::wininet",
        "diaguids::diaguids",
        "dbghelp::dbghelp",
        "normaliz::normaliz",
        "crypt32::crypt32",
        "mincore::mincore",
        "imagehlp::imagehlp",
    ]

    def license_for_target(self, tgt):
        return self.public_libs[self.internal_libs[tgt]]

    def is_licensed(self, tgt):
        return tgt in self.internal_libs

    def license_from_name(self, name):
        return self.public_libs[name]

    def license_closure_target(self, tgt):
        names = set()
        for tgs in self.targets[tgt]:
            if not self.is_licensed(tgs):
                raise Exception(
                    "You are distributing a target {} that relies on a depencency {} for which no license is declared. You need to:\n".format(
                        tgt, tgs
                    )
                    + "Declare it as a common local lib in this file {},".format(
                        __file__
                    )
                    + " or you will need to explicitly declare a license in the cmake files."
                )
            names.add(self.internal_libs[tgs])
        return names

    def licenses(self):
        return [self.license_from_name(x) for x in self.shipped_license_names]

    def __init__(self, notices, dependencies, installed_targets):
        self.public_libs, self.internal_libs = self._parse_notices(notices)
        self.deps = self._parse_dependencies(dependencies)

        # Mapping from target --> shipped binaries.
        self.target_exes = self._parse_installed_targets(installed_targets)

        # Target --> Dependency closure
        self.targets = {}
        for tgt in self.target_exes:
            self.targets[tgt] = self._target_closure(tgt, set())

        self.shipped_license_names = set()
        # Update exectuble sets for the licenses.
        for tgt, exes in self.target_exes.items():
            for lic in self.license_closure_target(tgt):
                self.shipped_license_names.add(lic)
                self.license_from_name(lic).register_exes(exes)

    def _parse_notices(self, notice):
        public_libs = {}
        internal_libs = {}
        with open(notice, "r") as nfile:
            for line in nfile.readlines():
                # ssl;crypto|boringssl|https://android.googlesource.com/platform/external/boringssl/+/refs/heads/master|OpenSSL|NOTICE|/Users/jansene/src/emu/external/qemu/../boringssl//NOTICE|
                alias, name, source, spdx, remote, local = [
                    x.strip() for x in line.split("|")
                ]
                if name == "":
                    continue

                for lib in [x.strip() for x in alias.split(";")]:
                    internal_libs[lib] = name

                public_libs[name] = License(name, source, spdx, remote, local)

        return public_libs, internal_libs

    def _parse_dependencies(self, deps):
        """Parses the direct dependencies.

        Returns a dictionary target -> targets
        """
        dependencies = {}
        with open(deps, "r") as nfile:
            for line in nfile.readlines():
                dep, _, liblist = [x.strip() for x in line.split("|")]
                dependencies[dep] = set(liblist.split(";"))
        return dependencies

    def _parse_installed_targets(self, targetfile):
        targets = defaultdict(set)
        with open(targetfile, "r") as nfile:
            for line in nfile.readlines():
                tgt, exe = [x.strip() for x in line.split(",")]
                targets[tgt].add(exe)
        return targets

    def _is_valid_target(self, target):
        return (
            target
            and (" " not in target)
            and (not target.startswith("-"))
            and ("NOTFOUND" not in target)
            and (target not in Licensing.common_local_libs)
        )

    def _target_closure(self, target, seen):
        """Calculates the dependency closure of the given target."""
        seen.add(target)
        deps = set()
        deps.add(target)
        if target in self.deps:
            for tgt in [x for x in self.deps[target] if self._is_valid_target(x)]:
                deps.add(tgt)
                if tgt not in seen:
                    deps = deps.union(self._target_closure(tgt, seen))

        return deps

    def to_csv(self):
        """Generates a csv file that can be used to export docker files."""
        csv = "External Library Name, Link to License, License Name, Binaries using the license.\n"
        for lic in self.licenses():
            if not lic.is_internal():
                csv += lic.to_csv()
                csv += "\n"

        return csv

    def notice_file(self):
        """Constructs the notices text."""
        notice = ""
        for lic in self.licenses():
            if not lic.is_internal():
                notice += (
                    "===========================================================\n"
                )
                notice += "Notices for file(s):\n"
                notice += "\n".join(sorted(lic.exes))
                notice += "\n"
                notice += "The notices is included for the library: {}\n\n".format(
                    lic.name
                )
                notice += (
                    "===========================================================\n"
                )
                notice += lic.read()
                notice += "\n\n"
        return notice
