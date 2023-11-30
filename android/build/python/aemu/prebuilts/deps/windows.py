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
import aemu.prebuilts.deps.common as common

def checkVSVersion(min_vers):
    """Checks the Visual Studio version against `min_vers`.
    Args:
        min_vers (tuple(int)): The minimum version required. Visual Studio version is 4-digits
        (e.g. 10.1111.10.0).

    Raises:
        Exception: If vswhere.exe is not found, or if the Visual Studio version does not meet the
        requirements.
    """
    vscheck = os.path.join(
        os.environ["PROGRAMFILES(X86)"], "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.isfile(vscheck):
        raise Exception("{} file not found. Unable to check VS version".format(vscheck))
    if not common.checkVersion(vers_cmd="{} -latest -property installationVersion".format(vscheck),
                        vers_regex="[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*",
                        min_vers=min_vers):
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