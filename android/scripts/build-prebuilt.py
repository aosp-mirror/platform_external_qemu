#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
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
import sys

def main():
  # This file should be located at
  #     external/qemu/android/scripts/build-<prebuilt>.py
  # for linux/mac, the script we want to use is at
  #     external/qemu/android/scripts/unix/build-<prebuilt>.sh
  # for windows, the script is at
  #     external/qemu/android/scripts/windows/build-<prebuilt>.bat
  #
  # Use symlinks to build whatever prebuilt you want. So for example, to build
  # curl, we need to create a file called android/scripts/build-curl.py that
  # symlinks to this file. If you want to build on linux/mac, you also need to
  # define a shell script called android/scripts/unix/build-curl.sh, and for
  # windows, android/scripts/windows/build-curl.bat.
  dir_path = os.path.dirname(os.path.realpath(__file__))

  filename = os.path.splitext(os.path.basename(sys.argv[0]))[0]
  if sys.platform == 'win32':
    fullname = os.path.join(dir_path, 'windows', filename + '.cmd');
  else:
    fullname = os.path.join(dir_path, 'unix', filename + '.sh');

  scriptWithArgs = [fullname] + sys.argv[1:]
  subprocess.call(scriptWithArgs)

if __name__ == '__main__':
  main()
