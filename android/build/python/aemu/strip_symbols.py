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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import sys
import urlfetch
import fnmatch
import os
import glob
import platform

from absl import app
from absl import flags
from absl import logging

from aemu.definitions import get_qemu_root, get_aosp_root
from aemu.process import run

FLAGS = flags.FLAGS
flags.DEFINE_string('install_dir', 'gradle-release',
                    'CMake install directory used')

flags.register_validator('install_dir',
                         lambda value:  os.path.exists(value), 'directory does not exist')


def recursive_iglob(rootdir, pattern_fns):
    for root, dirnames, filenames in os.walk(rootdir):
        for filename in filenames:
            fname = os.path.join(root, filename)
            for pattern_fn in pattern_fns:
                if pattern_fn(fname):
                    yield fname


# Functions that determine if a file is executable.
bin_map = {
    'Windows': [lambda f: f.endswith('.exe') or f.endswith('.dll')],
    'Linux': [lambda f: os.access(f, os.X_OK), lambda f: f.endswith('.so')],
    'Darwin': [lambda f: os.access(f, os.X_OK), lambda f: f.endswith('.dylib')],
}


def binaries():
    for bin in recursive_iglob(FLAGS.install_dir, bin_map[platform.system()]):
        logging.info("Checking %s", bin)


def main(argv=None):
    del argv  # Unused
    ## Not yet supported, we are missing breakpad symbol generators for the windows platform..
    if platform.system() == 'Windows':
        logging.error('Symbol stripping is not yet suppported on windows.. b/121210176')
    # revert to old mechanism
    cmd = [os.path.join(get_qemu_root()]





if __name__ == '__main__':
    app.run(main)
