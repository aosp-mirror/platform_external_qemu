#!/usr/bin/env python
#
# Copyright 2023 - The Android Open Source Project
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

import importlib.util
import json
import logging
import os
import platform
import re
import subprocess
import sys
from pathlib import Path

AOSP_ROOT = Path(__file__).resolve().parents[8]
EXE_SUFFIX = ".exe" if platform.system().lower() == "windows" else ""

def checkExeIsOnPath(exe):
    cmd = ["where", exe + EXE_SUFFIX] if platform.system().lower() == "windows" else ["which", exe + EXE_SUFFIX]
    try:
        res = subprocess.check_output(args=cmd, env=os.environ.copy(), encoding="utf-8").strip()
        logging.info("Found %s [%s]", exe, res)
        return True
    except subprocess.CalledProcessError:
        logging.critical("Could not locate %s", exe)
        return False

def checkVersion(vers_cmd, vers_regex, min_vers):
    """Checks the executable version against the version requirements.

    Args:
        vers_cmd (list(str)): A shell command to get the version information.
        vers_regex (str): The regular expression to filter for the version in vers_cmd output.
        min_vers (tuple(int)): The minimum required version. For example, version 12.0.1 = (12, 0, 1).

    Returns:
        bool: True if the version requirements are met, False otherwise.
    """
    try:
        res = subprocess.check_output(args=vers_cmd, env=os.environ.copy(), encoding="utf-8").strip()
        vers_str = re.search(vers_regex, res)
        logging.info("%s returned [%s], version=[%s]", vers_cmd, res, vers_str.group(0))
        vers = tuple(map(int, vers_str.group(0).split('.')))
        if vers < min_vers:
            logging.critical("%s returned [%s] is not at least version %s", vers_cmd, vers_str.group(0), min_vers)
            return False
    except subprocess.CalledProcessError:
        logging.critical("Encountered problem executing [%s]", vers_cmd)
        return False

    return True

def checkPythonVersion(min_vers):
    if sys.version_info < min_vers:
        raise Exception(f"Python version {sys.version} does not meet minimum requirements")

def checkExeVersion(exe, exe_version_arg, vers_regex="[0-9]*\.[0-9]*\.[0-9]*", min_vers=None):
    """Checks the version of an executable against `min_vers`.

    Args:
        exe (str): The name of the executable without suffix extension.
        exe_version_arg (str): The version argument(s) passed to `exe`.
        vers_regex (str): The regular expression used to filter for the version. Defaults to
        checking for a 3-digit version number (e.g. 1.12.3).
        min_vers (tuple(int), optional): The minimum required version. Defaults to None. If None,
        then this function only checks existence on the PATH.

    Raises:
        Exception: If executable is not on PATH, or does not meet the version requirements.
    """
    if not checkExeIsOnPath(exe):
        raise Exception(f"{exe} is not on PATH")
    if min_vers:
        if not checkVersion(vers_cmd=[exe + EXE_SUFFIX, exe_version_arg],
                            vers_regex=vers_regex,
                            min_vers=min_vers):
            raise Exception(f"{exe} does not meet the minimum requirements")

def checkPerlVersion(min_vers=None):
    checkExeVersion(exe="perl", exe_version_arg="--version", min_vers=min_vers)
def checkCmakeVersion(min_vers=None):
    checkExeVersion(exe="cmake", exe_version_arg="--version", min_vers=min_vers)

def checkNinjaVersion(min_vers=None):
    checkExeVersion(exe="ninja", exe_version_arg="--version", min_vers=min_vers)
def checkNodeJsVersion(min_vers=None):
    checkExeVersion(exe="node", exe_version_arg="--version",  min_vers=min_vers)
def checkBisonVersion(min_vers=None):
    checkExeVersion(exe="bison", exe_version_arg="--version", vers_regex="[0-9]*\.[0-9]*",
                    min_vers=min_vers)

def checkFlexVersion(min_vers=None):
    checkExeVersion(exe="flex", exe_version_arg="--version", vers_regex="[0-9]*\.[0-9]*",
                    min_vers=min_vers)

def checkGperfVersion(min_vers=None):
    checkExeVersion(exe="gperf", exe_version_arg="--version", vers_regex="[0-9]*\.[0-9]*",
                    min_vers=min_vers)

def checkPythonPackage(name):
    if importlib.util.find_spec(name) is None:
        raise Exception(f"Python package {name} not found.")

def getClangDirectory():
    # clang toolchain version is in external/qemu/android/build/toolchains.json
    toolchain_json_file = AOSP_ROOT / "external" / "qemu" / "android" / "build" / "toolchains.json"
    clang_path = AOSP_ROOT / "prebuilts" / "clang" / "host" / f"{platform.system().lower()}-x86"
    with open(toolchain_json_file, 'r') as f:
        toolchain_json = json.load(f)
        clang_path = clang_path / toolchain_json['clang']
    return clang_path

def addToSearchPath(searchDir):
    os.environ["PATH"] = searchDir + os.pathsep + os.environ["PATH"]