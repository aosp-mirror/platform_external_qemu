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

import logging
import os
import re
import subprocess

def checkMacOsSDKVersion(min_vers):
    vers_regex = "macosx([0-9]*\.[0-9]*)"
    try:
        res = subprocess.check_output(args=["xcodebuild", "-showsdks"], env=os.environ.copy(), encoding="utf-8").strip()
        vers_str = re.search(vers_regex, res)
        logging.info("xcodebuild -showsdk returned [%s], version=[%s]", res, vers_str.group(1))
        vers = tuple(map(int, vers_str.group(1).split('.')))
        if vers < min_vers:
            raise Exception(f"xcodebuild returned [{vers_str.group(1)}] is not at least version {min_vers}")
    except subprocess.CalledProcessError as e:
        logging.critical("Encountered problem executing xcodebuild -showsdks")
        raise e