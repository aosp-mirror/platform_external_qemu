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

from absl import app
from absl import flags
from absl import logging

import os
import platform
import shutil

import process
from definitions import Generator, Crash, BuildConfig, Toolchain, AOSP_ROOT, QEMU_ROOT, CMAKE_DIR

FLAGS = flags.FLAGS
flags.DEFINE_string('sdk_revision', None, 'The emulator sdk revision')
flags.DEFINE_string('sdk_build_number', None, 'The emulator sdk build number')
flags.DEFINE_enum('config', 'release', BuildConfig.values(),
                  'Build configuration for the emulator.')
flags.DEFINE_enum('crash', 'none', Crash.values(), 'Which crash server to use.')
flags.DEFINE_string('out', os.path.join(QEMU_ROOT,
                                        'objs'), 'Use specific output directory')
flags.DEFINE_boolean('qtwebengine', False, 'Build with QtWebEngine support')
flags.DEFINE_list('sanitizer', [], 'Build with asan sanitizer ([address, thread])')
flags.DEFINE_enum('generator', 'ninja', Generator.values(), 'Cmake generator to use')
flags.DEFINE_enum('target',  platform.system().lower(), Toolchain.values(),
                  'Which platform to target')


def configure():
    """COnfigures the cmake project."""
    # Clear out the existing directory.
    logging.info('Clearing out %s', FLAGS.out)
    shutil.rmtree(FLAGS.out)
    os.makedirs(FLAGS.out)

    # Configure..
    cmake_cmd = [get_cmake(), '-H%s' % QEMU_ROOT, '-B%s' % FLAGS.out]
    cmake_cmd += Toolchain.from_string(FLAGS.target).to_cmd()
    cmake_cmd += Crash.from_string(FLAGS.crash).to_cmd()
    cmake_cmd += BuildConfig.from_string(FLAGS.config).to_cmd()
    if FLAGS.qtwebengine:
        cmake_cmd += ['-DNO_QTWEBENGINE=1']
    cmake_cmd += ['-DOPTION_SDK_TOOLS_REVISION=%s' % FLAGS.sdk_revision]
    cmake_cmd += ['-DOPTION_SDK_TOOLS_BUILD_NUMBER=%s' %
                  FLAGS.sdk_build_number]
    cmake_cmd += ['-DOPTION_ASAN=%s' % (','.join(FLAGS.sanitizer))]
    cmake_cmd += Generator.from_string(FLAGS.generator).to_cmd()
    cmake_cmd = filter(None, cmake_cmd)

    process.run(cmake_cmd)


def get_cmake():
    exe = ''
    if platform.system() == 'Windows':
        exe = '.exe'

    return os.path.join(CMAKE_DIR, 'cmake%s' % exe)


def get_build_cmd():
    return [get_cmake(), '--build', FLAGS.out]


def main(argv):
    del argv  # Unused.

    # logging to logging.find_log_dir() folder
    version = sys.version_info
    logging.info('Running under Python {0[0]}.{0[1]}.{0[2]}'.format(version))
    configure()
    print('You can now build with: %s ' % ' '.join(get_build_cmd()))


if __name__ == '__main__':
    app.run(main)
