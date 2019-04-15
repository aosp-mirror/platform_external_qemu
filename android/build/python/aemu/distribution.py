#!/usr/bin/env python
#
# Copyright 2019 - The Android Open Source Project
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

from absl import logging

from aemu.definitions import get_qemu_root

import os
import re
import zipfile
import shutil

# These are the set of zip files that will be generated in the distribution directory.
# You can add additional zip sets here if you need. A file will be added to the given zipfile
# if a file under the --out directory matches the given regular expression.

# For naming you can use the {key} pattern, which will replace the given {key} with the value.
# Most of the absl FLAGS are available.
zip_sets = {
    'debug':  # Release type..
    {
        # Look for *.gcda files under the build tree
        'code-coverage-{target}.zip': ['{build_dir}', r'.*(gcda|gcno)'],
        # Look for all files under {out}/distribution
        'sdk-repo-{target}-emulator-full-debug-{sdk_build_number}.zip': ['{build_dir}/distribution', r'.*']
    },

    'release':
    {
        'sdk-repo-{target}-debug-emulator-{sdk_build_number}.zip': [
            # Look for all files under {out}/build/debug_info/
            '{build_dir}/build/debug_info', r'.*'
        ],
        'sdk-repo-{target}-grpc-samples-{sdk_build_number}.zip': ['{src_dir}/android/android-grpc/docs/grpc-samples/', r'.*'],
        # Look for all files under {out}/distribution
        'sdk-repo-{target}-emulator-{sdk_build_number}.zip': ['{build_dir}/distribution', r'.*']
    }
}


def recursive_glob(dir, regex):
    """Recursively glob the rootdir for any file that matches the given regex"""
    reg = re.compile(regex)
    for root, _, filenames in os.walk(dir):
        for filename in filenames:
            fname = os.path.join(root, filename)
            if reg.match(fname):
                yield fname


def _construct_notice_file(files, aosp):
    """Constructs a notice file for all the given individual files."""
    notice = '===========================================================\n'
    notice += 'Notices for file(s):\n'
    notice += '\n'.join(files)
    notice += '\n'
    notice += '===========================================================\n'
    with open(os.path.join(aosp, 'external', 'qemu', 'NOTICE'),
              'r') as nfile:
        notice += nfile.read()

    return notice


def create_distribution(dist_dir, build_dir, data):
    """Creates a release by copying files needed for the given platform."""
    if not os.path.exists(dist_dir):
        os.makedirs(dist_dir)

    # Let's find the set of
    zip_set = zip_sets[data['config']]

    # First we create the individual zip sets using the regular expressions.
    for zip_fname, params in zip_set.iteritems():
        zip_fname = os.path.join(dist_dir, zip_fname.format(**data))
        logging.info('Creating %s', zip_fname)

        files = []
        with zipfile.ZipFile(zip_fname, 'w', zipfile.ZIP_DEFLATED, allowZip64=True) as zipf:
            start_dir, regex = params
            search_dir = os.path.normpath(start_dir.format(build_dir = build_dir, src_dir = get_qemu_root()))
            for fname in recursive_glob(search_dir, regex.format(data)):
                arcname = fname[len(search_dir):]
                files.append(arcname)
                zipf.write(fname, arcname)

            # add a notice file.
            notice = _construct_notice_file(files, data['aosp'])
            zipf.writestr('emulator/NOTICE.txt', notice)
