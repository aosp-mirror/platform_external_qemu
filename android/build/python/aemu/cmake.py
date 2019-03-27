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
from aemu.definitions import Generator, Crash, BuildConfig, Toolchain, get_qemu_root, get_cmake, Make, get_aosp_root, fixup_windows_clang, read_simple_properties
from aemu.run_tests import run_tests
from aemu.distribution import create_distribution

FLAGS = flags.FLAGS
flags.DEFINE_string('sdk_revision', None,
                    '## DEPRECATED ##, it will automatically use the one defined in source.properties')
flags.DEFINE_string('sdk_build_number', None, 'The emulator sdk build number.')
flags.DEFINE_enum('config', 'release', BuildConfig.values(),
                  'Whether we are building a release or debug configuration.')
flags.DEFINE_enum('crash', 'none', Crash.values(),
                  'Which crash server to use or none if you do not want crash uploads.'
                  'enabling this will result in symbol processing and uploading during install.')
flags.DEFINE_string('out', os.path.abspath('objs'),
                    'Use specific output directory.')
flags.DEFINE_string('dist', None, 'Create distribution in directory')
flags.DEFINE_boolean('qtwebengine', False, 'Build with QtWebEngine support')
flags.DEFINE_boolean('tests', True, 'Run all the tests')
flags.DEFINE_list(
    'sanitizer', [], 'List of sanitizers ([address, thread]) to enable in the built binaries.')
flags.DEFINE_enum('generator', 'ninja', Generator.values(),
                  'CMake generator to use.')
flags.DEFINE_enum('target',  platform.system().lower(), Toolchain.values(),
                  'Which platform to target. '
                  'This will attempt to cross compile '
                  'if the target does not match the current platform (%s)' % platform.system().lower())
flags.DEFINE_enum('build', 'check', Make.values(),
                  'Target that should be build after configuration. '
                  'The config target will only configure the build, '
                  'no symbol processing or testing will take place.')
flags.DEFINE_boolean(
    'clean', True, 'Clean the destination build directory before configuring. '
    'Setting this to false will attempt an incremental build. '
    'Note that this can introduce cmake caching issues.')
flags.DEFINE_multi_string('cmake_option', [], 'Options that should be passed '
                          'on directly to cmake. These will be passed on directly '
                          'to the underlying cmake project. For example: '
                          '--cmake_option QEMU_UPSTREAM=FALSE')

def configure():
    """Configures the cmake project."""

    # Clear out the existing directory.
    if FLAGS.clean:
        if os.path.exists(FLAGS.out):
            logging.info('Clearing out %s', FLAGS.out)
            shutil.rmtree(FLAGS.out)
        if not os.path.exists(FLAGS.out):
            os.makedirs(FLAGS.out)

    # Configure..
    cmake_cmd = [get_cmake(), '-B%s' % FLAGS.out]

    # Setup the right toolchain/compiler configuration.
    cmake_cmd += Toolchain.from_string(FLAGS.target).to_cmd()
    cmake_cmd += Crash.from_string(FLAGS.crash).to_cmd()
    cmake_cmd += BuildConfig.from_string(FLAGS.config).to_cmd()
    cmake_cmd += ['-DQTWEBENGINE=%s' % FLAGS.qtwebengine]

    if FLAGS.cmake_option:
        flags = ['-D%s' % x for x in FLAGS.cmake_option]
        logging.warn('Dangerously adding the following flags to cmake: %s', flags)
        cmake_cmd += flags

    if FLAGS.sdk_revision:
        sdk_revision = FLAGS.sdk_revision
    else:
        sdk_revision = read_simple_properties(os.path.join(
            get_aosp_root(), 'external', 'qemu', 'source.properties'))['Pkg.Revision']
    cmake_cmd += ['-DOPTION_SDK_TOOLS_REVISION=%s' % sdk_revision]

    if FLAGS.sdk_build_number:
        cmake_cmd += ['-DOPTION_SDK_TOOLS_BUILD_NUMBER=%s' %
                      FLAGS.sdk_build_number]
    if FLAGS.sanitizer:
        cmake_cmd += ['-DOPTION_ASAN=%s' % (','.join(FLAGS.sanitizer))]

    cmake_cmd += Generator.from_string(FLAGS.generator).to_cmd()
    cmake_cmd += [get_qemu_root()]

    # Make sure we fixup clang in windows builds
    if platform.system() == 'Windows':
        fixup_windows_clang()

    run(cmake_cmd, FLAGS.out)


def get_build_cmd():
    '''Gets the command that will build all the sources.'''
    target = 'install'
    # Stripping is meaning less in windows world.
    if FLAGS.crash != 'none' and platform.system().lower() != 'windows':
        target += '/strip'
    return [get_cmake(), '--build', FLAGS.out, '--target', target]


def main(argv=None):
    del argv  # Unused.

    version = sys.version_info
    logging.info('Running under Python {0[0]}.{0[1]}.{0[2]}, Platform: {1}'.format(
        version, platform.platform()))

    configure()
    if FLAGS.build == 'config':
        print('You can now build with: %s ' % ' '.join(get_build_cmd()))
        return

    # Build
    run(get_build_cmd())

    # Test.
    if FLAGS.tests:
        run_tests(FLAGS.out)

    # Create a distribution if needed.
    if FLAGS.dist:
        data = {'aosp': get_aosp_root(),
                'target': FLAGS.target,
                'sdk_build_number': FLAGS.sdk_build_number,
                'config': FLAGS.config
                }
        if FLAGS.target == 'mingw':
            data['target'] = 'windows'

        create_distribution(FLAGS.dist, FLAGS.out, data)

    if platform.system() != 'Windows' and FLAGS.config == 'debug':
        overrides = open(os.path.join(
            get_qemu_root(), 'android', 'asan_overrides')).read()
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
