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

from aemu.process import run
from aemu.definitions import Generator, Crash, BuildConfig, SymbolUris, Toolchain, get_qemu_root, get_cmake, Make
from aemu.run_tests import run_tests
from aemu.upload_symbols import upload_symbols

FLAGS = flags.FLAGS
flags.DEFINE_enum('symbol_dest', 'none', SymbolUris.values(),
                  'Symbol server, if we are processing symbols.')
flags.DEFINE_string('sdk_revision', None, 'The emulator sdk revision')
flags.DEFINE_string('sdk_build_number', None, 'The emulator sdk build number')
flags.DEFINE_enum('config', 'release', BuildConfig.values(),
                  'Build configuration for the emulator.')
flags.DEFINE_enum('crash', 'none', Crash.values(),
                  'Which crash server to use.')
flags.DEFINE_string('out', os.path.abspath('objs'),
                    'Use specific output directory')
flags.DEFINE_boolean('qtwebengine', False, 'Build with QtWebEngine support')
flags.DEFINE_list(
    'sanitizer', [], 'Build with sanitizer ([address, thread])')
flags.DEFINE_enum('generator', 'ninja', Generator.values(),
                  'Cmake generator to use')
flags.DEFINE_enum('target',  platform.system().lower(), Toolchain.values(),
                  'Which platform to target')
flags.DEFINE_enum('build', 'check', Make.values(),
                  'Target we want to build, if any')
flags.DEFINE_boolean('clean', True, 'Clean the destination build directory')
flags.DEFINE_boolean('symbols', False, 'Strip and generate symbols')


def configure():
    """Configures the cmake project."""

    # Clear out the existing directory.
    if FLAGS.clean:
        logging.info('Clearing out %s', FLAGS.out)
        if os.path.exists(FLAGS.out):
            shutil.rmtree(FLAGS.out)
        if not os.path.exists(FLAGS.out):
            os.makedirs(FLAGS.out)

    # Configure..
    cmake_cmd = [get_cmake(), '-H%s' % get_qemu_root(), '-B%s' % FLAGS.out]
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

    run(cmake_cmd)


def get_build_cmd():
    '''Gets the command that will build all the sources.'''
    return [get_cmake(), '--build', FLAGS.out, '--target', 'install']


def strip_symbols(out_dir):
    '''Strip all the symbols.'''
    if platform.system() == 'Windows':
        logging.error(
            'Symbol stripping is not yet suppported on windows.. b/121210176')
        return

    # revert to old mechanism as we do not yet have support on all platforms.
    cmd = [os.path.join(get_qemu_root(), 'android', 'scripts', 'strip-symbols.sh'),
           '--out-dir=%s' % out_dir]
    if FLAGS.target == 'windows' or FLAGS.target == 'mingw':
        cmd.append('--mingw')
    run(cmd)


def main(argv=None):
    del argv  # Unused.

    version = sys.version_info
    logging.info('Running under Python {0[0]}.{0[1]}.{0[2]}, Platform: {1}'.format(
        version, platform.platform()))

    configure()
    if FLAGS.build == Make.config:
        print('You can now build with: %s ' % ' '.join(get_build_cmd()))
        return

    # Build
    run(get_build_cmd())

    # Test.
    run_tests(FLAGS.out)

    # Strip symbols
    if FLAGS.symbols:
        logging.info("Stripping symbols")
        strip_symbols(FLAGS.out)

    # Upload the symbols..
    uri = SymbolUris.from_string(FLAGS.symbol_dest).value
    if uri:
        upload_symbols(FLAGS.out, uri)

    if platform.system() != 'Windows' and FLAGS.config == 'debug':
        overrides = open(os.path.join(get_qemu_root(), 'android', 'asan_overrides')).read()
        print("Debug build enabled.")
        print("ASAN may be in use; recommend disabling some ASAN checks as build is not")
        print("universally ASANified. This can be done with")
        print()
        print(". android/envsetup.sh")
        print("")
        print("or export ASAN_OPTIONS=%s" % overrides)



def launch():
    app.run(main)


if __name__ == '__main__':
    launch()
