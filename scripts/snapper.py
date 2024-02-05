#!/usr/bin/env python
#
# Copyright 2024 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
#
# This is a command line tool to enumerate or delete Android Emulator
# snapshots without the need to launch the emulator itself.
#
#
# Usage examples:
#
# 'python snapper -l'               : Lists a detailed view of all existing
#                                   snapshots across all available avds
# 'python snapper -l --avd Pixel7'  : Lists a detailed view of all existing
#                                   snapshots for the avd named Pixel7
# 'python snapper -d --avd Pixel7'  : Deletes all existing snapshots for the
#                                   avd named Pixel7
# 'python snapper -d --snapshot default_boot' : Deletes all existing snapshots
#                                   named default_boot across all available avds
# 'python snapper -d --avd Pixel7 --snapshot default_boot' : Deletes snapshot
#                                   named defaut_boot from avd named Pixel7
#
# 'python snapper.py --help' for more details


import argparse
import logging
import os
from pathlib import Path
import shutil
import subprocess
import sys

# NOTE: Needs to be adjusted depending on your setup
# TODO: Replace these with known env variables...
# We do check the validity of these paths on startup.
QEMU_IMG = '~/emu/master/external/qemu/objs/qemu-img'
AVD_HOME = '~/.android/avd/'


DOT_QCOW = '.qcow2'
DOT_AVD  = '.avd'
SNAP_DIR = 'snapshots'

def qemu_img_snapshot_list(avd_path):
    """Runs the qemu-img command to print a list of snapshots."""

    avd_path = Path(avd_path)
    for filename in avd_path.glob(f'*{DOT_QCOW}'):
        command = [QEMU_IMG, 'snapshot', '-l',
                   os.path.join(str(avd_path), filename)]
        logging.debug(command)

        result = subprocess.run(command, capture_output = True, text = True)
        print(result.stdout)
        print(result.stderr)

        # We used the first qcow2 to list the snapshots, skip the rest.
        return

def qemu_img_snapshot_delete(avd_path, snapshot):
    """Runs the qemu-img command to delete the specified snapshots"""

    avd_path = Path(avd_path)
    for filename in avd_path.glob(f'*{DOT_QCOW}'):
        command = [QEMU_IMG, 'snapshot', '-d', snapshot,
                   os.path.join(str(avd_path), filename)]
        logging.debug(command)

        result = subprocess.run(command, capture_output = True, text = True)
        logging.debug(result.stdout)
        logging.debug(result.stderr)

    snap_path = os.path.join(avd_path, SNAP_DIR, snapshot)
    if os.path.isdir(snap_path):
        shutil.rmtree(snap_path)

def extract_snapshot_names(avd_path):
    """Returns a list of all the snapshots for the given avd.

    Internally it uses the qemu-img list command and parses the output.
    """

    snapshots = []
    avd_path = Path(avd_path)
    for filename in avd_path.glob(f'*{DOT_QCOW}'):
        command = [QEMU_IMG, 'snapshot', '-l',
                   os.path.join(str(avd_path), filename)]
        logging.debug(command)

        result = subprocess.run(command, capture_output = True, text = True)
        for line in result.stdout.splitlines():
            splits = line.split()
            if (len(splits) < 5
                or splits[0].startswith('Snapshot')
                or splits[0].startswith('ID')):
                continue
            snapshots.append(splits[1])

        return snapshots

def list_snapshots(avd):
    """Lists all the matching snapshots.

    Args:
        avd: The name of an avd to filter the snapshot list with. If empty, it
            will list all the existing snapshots from all available avds.
    """

    if avd:
        avd_path = os.path.join(AVD_HOME, avd + DOT_AVD)
        if not os.path.isdir(avd_path):
            raise NotADirectoryError(f'Could not find {avd_path}')

        qemu_img_snapshot_list(avd_path)
    else:
        avd_home = Path(AVD_HOME)
        for filename in avd_home.glob(f'*{DOT_AVD}'):
            print(str(filename)[:-len(DOT_AVD)] + ':')
            qemu_img_snapshot_list(os.path.join(AVD_HOME, filename))

def delete_snapshots(avd, snapshot):
    """Deletes all the matching snapshots.

    Args:
        avd: The name of an avd to filter the snapshots. If empty, it will
            consider all the existing avds. If avd is provided without a
            snapshot name, all the snapshots for that avd will be deleted.
        snapshot: The name of a snapshot to be deletet. Either a snapshot or
            an avd name (or both) must be provided.
    """

    if avd:
        avd_path = os.path.join(AVD_HOME, avd + DOT_AVD)
        if not os.path.isdir(avd_path):
            raise NotADirectoryError(f'Could not find {avd_path}')

        if snapshot:
            qemu_img_snapshot_delete(avd_path, snapshot)
        else:
            for snap in extract_snapshot_names(avd_path):
                qemu_img_snapshot_delete(avd_path, snap)
    else:
        # Avd is empty, it is safe to assume snapshot is not.
        avd_home = Path(AVD_HOME)
        for filename in avd_home.glob(f'*{DOT_AVD}'):
            qemu_img_snapshot_delete(os.path.join(AVD_HOME, filename),
                                     snapshot)

def validate_paths_or_throw():
    """Validates the qemu-img bindary and avd home paths.

       Will raise an exception if invalid.
    """

    global QEMU_IMG
    QEMU_IMG = os.path.expanduser(QEMU_IMG)
    if not os.path.isfile(QEMU_IMG):
        raise FileNotFoundError(f'Could not find {QEMU_IMG}')

    global AVD_HOME
    AVD_HOME = os.path.expanduser(AVD_HOME)
    if not os.path.isdir(AVD_HOME):
        raise NotADirectoryError(f'Could not find {AVD_HOME}')

def validate_args_or_throw(args):
    """Validates the input command and its args.

       Will raise an exception if invalid.
    """

    if not args.list and not args.delete:
        raise ValueError('Consider using --list or --delete commands.')
    elif args.list and args.delete:
        raise ValueError('Cannot use --list and --delete commands at the same'
                         ' time.')
    elif args.list and not args.avd:
        logging.warning('Avd was not specified. listing snapshots from all'
                        ' available avds:')
    elif args.list and args.snapshot:
        logging.warning('--snapshot will be ignored.')
    elif args.delete and not args.avd and not args.snapshot:
        raise ValueError('Must provide an avd and/or a snapshot with the'
                         ' delete command.')
    print()

    return args

def init_args_parser():
    parser = argparse.ArgumentParser(description='A command line tool to list '
                                     'and delete Android emulator snapshots')
    parser.add_argument('-l', '--list', action='store_true',
                        help="list of exisitng snapshots")
    parser.add_argument('-d', '--delete', action='store_true',
                        help="delete a snapshot")
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='print the underlying executed commands and '
                        'parameters.')
    parser.add_argument('--avd', type=str,
                        help='avd name to pick the snapshots; if not specified'
                        ' it will consider all existing avds.')
    parser.add_argument('--snapshot', type=str, help='snapshot name to delete;'
                        ' if not specified it will consider all existing'
                        ' snapshots.')

    return parser

def main(argv=None):
    if argv is None:
        argv = sys.argv

    validate_paths_or_throw()
    parser = init_args_parser()
    args = validate_args_or_throw(parser.parse_args())

    logging.basicConfig(
        level=(logging.DEBUG if args.verbose else logging.WARNING))

    if args.list:
        list_snapshots(args.avd)
    elif args.delete:
        delete_snapshots(args.avd, args.snapshot)

    return 0

if __name__ == '__main__':
    sys.exit(main())