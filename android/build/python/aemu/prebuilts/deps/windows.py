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

import os
import subprocess
import aemu.prebuilts.deps.common as common

def checkVSVersion(min_vers):
    """Checks the Visual Studio version against `min_vers`.
    Args:
        min_vers (tuple(int)): The minimum version required. Visual Studio version is 4-digits
        (e.g. 10.1111.10.0).

    Raises:
        Exception: If the Visual Studio version does not meet the version requirements.
    """
    vsVersionEnv = os.environ["VISUALSTUDIOVERSION"]
    vs_version = tuple(map(int, vsVersionEnv.split('.')))
    if vs_version < min_vers:
        raise Exception("Visual Studio version does not meet requirements")

def checkWindowsSdk(min_vers):
    """Checks the Windows SDK version against `min_vers`.
    Args:
        min_vers (tuple(int)): The minimum version required. Windows SDK version is 4-digits
        (e.g. 10.1111.10.0).

    Raises:
        Exception: If the Windows SDK does not meet the requirements.
    """
    # WindowsSDKVersion may have a trailing '\' character
    windowsSdkVersionEnv = os.environ["WINDOWSSDKVERSION"].split('\\')[0]
    winsdk_version = tuple(map(int, windowsSdkVersionEnv.split('.')))
    if winsdk_version < min_vers:
        raise Exception("Windows SDK does not meet requirements")

def inheritSubprocessEnv(cmd):
    """Runs `cmd` in a subprocess, then copies the subprocess environment variables to os.environ.

    Args:
        cmd (list(str)): The shell command to execute.
    """
    res = subprocess.check_output(cmd + ["&", "set"], env=os.environ.copy(), encoding="utf-8")
    for line in res.splitlines():
        key, value = line.split("=", 1)
        os.environ[key] = value