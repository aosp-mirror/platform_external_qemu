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

import os
import platform
import sys

from absl import app
from absl import flags
from absl import logging

from definitions import CMAKE_DIR, QEMU_ROOT
from process import run


FLAGS = flags.FLAGS
flags.DEFINE_string('out', os.path.join(QEMU_ROOT,
                                        'objs'), 'Use specific output directory')

EXE = ''
if platform.system() == 'Windows':
    EXE = '.exe'


def main(argv):
    del argv  # Unused.
    if platform.system() == 'Windows':
        run_binary_exists()
        run_emugen_test()
        run_ctest()
    else:
        run(os.path.join(QEMU_ROOT, 'android', 'scripts',
        run))


def run_binary_exists():
    if not os.exists(os.path.join(FLAGS.out, 'emulator%s' % EXE)):
        raise Exception('Emulator binary is missing')


def run_ctest():
    cmd = [get_ctest(), '--output-on-failure']
    run(cmd, FLAGS.out)


def run_emugen_test():
    emugen = os.path.abspath(os.path.join(FLAGS.out, 'emugen%s' % EXE))
    if platform.system() != 'Windows':
        cmd = [os.path.join(QEMU_ROOT, 'android', 'android-emugl',
                            'host', 'tools', 'emugen', 'tests', 'run-tests.sh'),
               '--emugen=%s' % emugen]
        run(cmd, FLAGS.out)
    else:
        logging.info('gen_tests not supported on windows yet.')


def get_ctest():
    '''Return the path of ctest executable.'''
    return os.path.join(CMAKE_DIR, 'ctest%s' % EXE)


if __name__ == '__main__':
    app.run(main)
