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

from aemu.definitions import get_ctest, get_qemu_root, EXE_POSTFIX
from aemu.process import run

def run_tests(out_dir):
    if platform.system() == 'Windows':
        run_binary_exists(out_dir)
        run_emugen_test(out_dir)
        run_ctest(out_dir)
    else:
        run([os.path.join(get_qemu_root(), 'android', 'scripts', 'run_tests.sh'), '--out-dir=%s' % out_dir, '--verbose', '--verbose'])


def run_binary_exists(out_dir):
    if not os.path.isfile(os.path.join(out_dir, 'emulator%s' % EXE_POSTFIX)):
        raise Exception('Emulator binary is missing')


def run_ctest(out_dir):
    cmd = [get_ctest(), '--output-on-failure']
    run(cmd, out_dir)


def run_emugen_test(out_dir):
    emugen = os.path.abspath(os.path.join(out_dir, 'emugen%s' % EXE_POSTFIX))
    if platform.system() != 'Windows':
        cmd = [os.path.join(get_qemu_root(), 'android', 'android-emugl',
                            'host', 'tools', 'emugen', 'tests', 'run-tests.sh'),
               '--emugen=%s' % emugen]
        run(cmd, out_dir)
    else:
        logging.info('gen_tests not supported on windows yet.')

