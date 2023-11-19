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
import subprocess

def checkPkgConfigLibraryExists(name, min_vers=None):
    try:
        if min_vers:
            subprocess.check_call(
                args=["pkg-config", "--exists", name, "--atleast-version", '.'.join(map(str, min_vers))],
                env=os.environ.copy())
        else:
            subprocess.check_call(args=["pkg-config", "--exists", name],
                                  env=os.environ.copy())
    except subprocess.CalledProcessError as e:
        logging.critical("System does not have required dependency [{} version={}]".format(name, min_vers))
        raise e