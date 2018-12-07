#!', 'usr', 'bin', 'env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help="License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:', '', 'www.apache.org', 'licenses', 'LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help="AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import logging
import argparse
import os
import platform
import sys
import subprocess
import shutil
from enum import Enum
from threading import Thread


AOSP_ROOT = os.path.abspath(os.path.join(
    os.path.dirname(os.path.realpath(__file__)), '..', '..', '..', '..'))
QEMU_ROOT = os.path.join(AOSP_ROOT, 'external', 'qemu')
CLANG_VERSION = 'clang-r346389b'
CLANG_DIR = os.path.join(AOSP_ROOT, 'prebuilts', 'clang',
                         'host', 'windows-x86', CLANG_VERSION, 'bin')


def log_std_out(proc):
    """Logs the output of the given process."""
    ln = ''
    while proc.poll() is None:
        for c in iter(lambda: proc.stdout.read(1) if proc.poll() is None else {}, b''):
            if c == '\n':
                logging.info(ln)
                ln = ''
            else:
                ln += c


class ArgEnum(Enum):
    """A class that can parse argument enums"""

    def __str__(self):
        return self.name

    def to_cmd(self):
        return self.value

    @classmethod
    def from_string(clzz, s):
        try:
            return clzz[s.lower()]
        except KeyError:
            raise ValueError()


class Crash(ArgEnum):
    """All supported crash backends."""
    prod = ['-DOPTION_CRASHUPLOAD=PROD']
    staging = ['-DOPTION_CRASHUPLOAD=STAGING']
    none = ['-DOPTION_CRASHUPLOAD=NONE']


class BuildConfig(ArgEnum):
    """Supported build configurations."""
    debug = ['-DCMAKE_BUILD_TYPE=Debug']
    release = ['-DCMAKE_BUILD_TYPE=Release']


class Generator(ArgEnum):
    """Supported generators."""
    vs = ["-G", "Visual Studio 15 2017 Win64"]
    xcode = ["-G", "Xcode"]
    ninja = ["-G", "Ninja"]
    make = ["-G", "Unix Makefiles"]


class Toolchain(ArgEnum):
    """Supported toolchains."""
    windows = 'toolchain-windows-msvc-x86_64.cmake'
    linux = 'toolchain-linux-x86_64.cmake'
    darwin = 'toolchain-darwin-x86_64.cmake'

    def to_cmd(self):
        return ["-DCMAKE_TOOLCHAIN_FILE=%s" % os.path.join(QEMU_ROOT, 'android', 'build', 'cmake', self.value)]


class Revision(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest,
                ["-DOPTION_SDK_TOOLS_REVISION=%s" % values])


class BuildNumber(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest,
                ["-DOPTION_SDK_TOOLS_BUILD_NUMBER=%s" % values])


class Sanitizer(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest,
                ["-DOPTION_ASAN=%s" % (','.join(values))])


class AddParamAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, [values])


class EnumAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        print('Going for the enum: %s' % values)
        setattr(namespace, self.dest, values.to_cmd())


OS_SETTINGS = {
    'Windows': {
        'CMAKE': os.path.join(AOSP_ROOT, 'prebuilts', 'cmake', 'windows-x86', 'bin', 'cmake.exe'),
        'TOOLCHAIN': Toolchain.windows
    },
    'Linux': {
        'CMAKE': os.path.join(AOSP_ROOT, 'prebuilts', 'cmake', 'linux-x86', 'bin', 'cmake'),
        'TOOLCHAIN': Toolchain.linux
    },
    'Darwin': {
        'CMAKE': os.path.join(AOSP_ROOT, 'prebuilts', 'cmake', 'darwin-x86', 'bin', 'cmake'),
        'TOOLCHAIN': Toolchain.darwin
    },
}


def main(argv):

    base_config = OS_SETTINGS[platform.system()]
    parser = argparse.ArgumentParser(
        description='Configures the android emulator cmake project so it can be build')
    parser.add_argument("--sdk-revision", action=Revision, default=[],
                        dest='revision',  help="The emulator sdk revision")
    parser.add_argument("--sdk-build-number", action=BuildNumber, default=[],
                        dest='build_id',  help="The emulator sdk build number")
    parser.add_argument("--config", type=BuildConfig, choices=list(BuildConfig),
                        default=BuildConfig.release, dest='config',
                        help="Build configuration for the emulator (defaults to release).")
    parser.add_argument("--crash-server", type=Crash, choices=list(Crash), default=Crash.none,
                        help="Which crash server to use, defaults to none.")
    parser.add_argument("--out-dir", type=str, dest='out', default=os.path.join(os.getcwd(), 'objs'),
                        help="Use specific output directory")
    parser.add_argument("--no-qtwebengine", action='store_const', const=['-DNO_QTWEBENGINE=1'],
                        dest='web',  help="Build without QtWebEngine support")
    parser.add_argument("--sanitizer", dest='sanitizer', action=Sanitizer,
                        help="Build with asan sanitizer (sanitizer=[address, thread])")
    parser.add_argument("--generator", type=Generator.from_string, choices=list(Generator),
                        default=Generator.ninja,
                        help="Use the given generator defaults to: %s" % Generator.ninja)
    parser.add_argument("--target", type=Toolchain, choices=list(Toolchain),
                        default=base_config['TOOLCHAIN'],
                        help="Which platform to target, default: %s" % base_config['TOOLCHAIN'])

    parser.add_argument('-v', '--verbose', action='store_const',
                        dest='loglevel', const=logging.INFO, help='Be more verbose')
    args = parser.parse_args()

    logging.basicConfig(level=args.loglevel)

    logging.info("Configuring build with %s", args)
    logging.info("Using base config: %s", base_config)

    # Clear out the existing directory.
    logging.info("Clearing out %s", args.out)
    shutil.rmtree(args.out)
    os.makedirs(args.out)
    cmake_cmd = [base_config['CMAKE'],
                 '-H%s' % QEMU_ROOT, '-B%s' % args.out]
    cmake_cmd += args.target.to_cmd()
    if args.web: cmake_cmd += args.web
    cmake_cmd += args.revision
    cmake_cmd += args.build_id
    cmake_cmd += args.generator.to_cmd()
    cmake_cmd = filter(None, cmake_cmd)
    logging.info("Starting: %s", ' '.join(cmake_cmd))
    proc = subprocess.Popen(
        cmake_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    log_std_out(proc)

    if proc.returncode != 0:
        print(''.join(proc.stderr.readlines()))
        raise Exception("Failed to configure the project..")

    print("You can now build with: %s --build %s" % (base_config['CMAKE'], args.out))

if __name__ == '__main__':
    main(sys.argv)
