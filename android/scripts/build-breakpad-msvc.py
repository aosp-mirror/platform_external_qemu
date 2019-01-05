#!/usr/bin/env python3
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
import os
import re
import tempfile
import urlfetch
import shutil
import tarfile
import platform
import patch

# Setup so we can find the build additions.
root = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(
    __file__)), '..', '..'))
sys.path.append(os.path.join(root, 'android', 'build', 'python'))
sys.path.append(os.path.join(root, 'android', 'third_party', 'chromium', 'depot_tools'))


from absl import app
from absl import flags
from absl import logging

from aemu.process import run
from aemu.definitions import get_qemu_root, get_aosp_root, recursive_iglob


# Note, if you run under python27 you will need pylzma, which might be hard to come by..
# It ships in python3..
try:
    import lzma as xz
except ImportError:
    import pylzma as xz  # backport from python2


FLAGS = flags.FLAGS
flags.DEFINE_string('build_dir', os.path.abspath(os.path.join(tempfile.gettempdir(), 'breakpad')),
                    'Use specific build directory.')


def simple_parse_packages():
    packages_txt = os.path.join(get_aosp_root(
    ), 'prebuilts', 'android-emulator-build', 'archive', 'PACKAGES.TXT')
    packages = {}
    with open(packages_txt, 'r') as pkg_file:
        lines = pkg_file.readlines()
        for line in lines:
            entry = dict(re.findall(r'(\S+)=([^\s]+)', line))
            # We only parse entries with BASENAME, as we don't really care about all.
            if 'BASENAME' in entry:
                packages[entry['BASENAME']] = entry
    return packages


def extract_lzma(src, dst):
    '''Extract the source to dest in 4k chunks.'''
    logging.info("Extracting %s -> %s", src, dst)
    dst_name = src[:-3]
    with xz.open(src) as src_f:
        with open(dst_name, 'wb') as dst_f:
            for chunk in iter(lambda: src_f.read(4096), b''):
                dst_f.write(chunk)
    with tarfile.open(dst_name) as tar:
         tar.extractall(dst)

def get_package_archive(name):
    return os.path.join(get_aosp_root(
    ), 'prebuilts', 'android-emulator-build', 'archive', name)


def get_breakpad():
    packages = simple_parse_packages()
    breakpad = packages['breakpad']
    googletest = packages['googletest']
    breakpad_file = get_package_archive('breakpad-%s.tar.xz' % breakpad['BRANCH'])
    breakpad_patches = get_package_archive(breakpad['PATCHES'])
    googletest_file = get_package_archive('googletest-%s.tar.xz' % googletest['BRANCH'])
    extract_lzma(breakpad_file, FLAGS.build_dir)
    extract_lzma(breakpad_patches, FLAGS.build_dir)
    extract_lzma(googletest_file, FLAGS.build_dir)


    breakpad_dst_dir = os.path.join(FLAGS.build_dir, 'breakpad-%s' % breakpad['BRANCH'])
    breakpad_patch_dst_dir = os.path.join(FLAGS.build_dir, 'breakpad-%s-patches' % breakpad['BRANCH'])
    os.rename(os.path.join(FLAGS.build_dir, 'googletest-%s' % googletest['BRANCH']), os.path.join(breakpad_dst_dir, 'src', 'testing'))
    is_patch = lambda x : x.endswith('patch')
    for patchfile in recursive_iglob(breakpad_patch_dst_dir, [is_patch]):
        logging.info("Applying %s in %s", patchfile, breakpad_dst_dir)
        pat = patch.fromfile(patchfile)
        pat.apply(0, breakpad_dst_dir)

    return breakpad_dst_dir


def build_windir(win_build, gyp_file):
    sln = os.path.join(win_build, '%s.sln' % gyp_file)
    if os.path.exists(sln):
        os.remove(sln)
    run(['gyp', '%s.gyp' % gyp_file,
         '--no-circular-check'], cwd=win_build)
    run(['msbuild', '%s.sln' % gyp_file, '/p:Configuration=Release',
         '/t:Clean,Build', '/m'], cwd=win_build)
    return os.path.join(win_build, 'Release')


def build_for_windows(src_dir):
    dest = os.path.join(get_aosp_root(), 'prebuilts', 'android-emulator-build',
                        'common', 'breakpad', 'windows_msvc-x86_64')

    # First build the tools.
    win_build = os.path.join(src_dir,
                              'src', 'tools', 'windows')
    rel_dir = build_windir(win_build, 'tools_windows')

    shutil.copyfile(os.path.join(rel_dir, 'symupload.exe'),
                    os.path.join(dest, 'bin', 'symupload.exe'))
    shutil.copyfile(os.path.join(rel_dir, 'dump_syms.exe'),
                    os.path.join(dest, 'bin', 'dump_syms.exe'))

    # Next build the client.
    win_build = os.path.join(src_dir, 'src', 'client', 'windows')
    rel_dir = build_windir(win_build, 'breakpad_client')

    # Next we install the libs.
    lib_dir = os.path.join(dest, 'lib')
    for lib in ['crash_generation_client.lib', 'crash_generation_server.lib', 'crash_report_sender.lib', 'exception_handler.lib', 'processor_bits.lib']:
        shutil.copyfile(os.path.join(rel_dir, 'lib', lib),
                        os.path.join(lib_dir, lib))

    # Set of directories we care about. Note that the MSBuild doesn't have an install target, so this is really best effort here :-(
    install_dirs = [
        "google_breakpad",
        "google_breakpad\\processor",
        "google_breakpad\\common",
        "client",
        "client\\windows",
        "client\\windows\\handler",
        "client\\windows\\common",
        "client\\windows\\crash_generation",
        "processor",
        "common",
        "common\\windows",
    ]

    for inc_dir in install_dirs:
        header_src = os.path.join(src_dir, 'src', inc_dir)
        header_dst = os.path.join(dest, 'include', 'breakpad', inc_dir)
        headers = [f for f in os.listdir(header_src) if f.endswith('.h')]
        for header in headers:
            logging.info("Copying %s -> . %s", header, header_dst)
            shutil.copyfile(os.path.join(header_src, header), os.path.join(header_dst, header))


def main(argv=None):
    del argv  # Unused.

    # First obtain breakpad from gclient.
    if platform.system() != 'Windows':
        print("You should be using the build-breakpad.sh script to build on linux/darwin.")
        sys.exit(-1)

    if os.path.exists(FLAGS.build_dir):
        logging.info('Clearing out %s', FLAGS.build_dir)
        shutil.rmtree(FLAGS.build_dir)

    src_dir = get_breakpad()
    build_for_windows(src_dir)


def launch():
    app.run(main)


if __name__ == '__main__':
    launch()
