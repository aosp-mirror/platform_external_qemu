#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""A basic emulator launcher."""

import os
import glob
import socket
import subprocess
import shutil
import time
import psutil

from absl import logging

class Emulator(object):

    def __init__(self, emulator_exe, max_boot, avd = None):
        self.emulator = emulator_exe
        self.max_boot = max_boot
        self.avd = avd or self.get_avd()
        self.adb_port = self._unsafe_get_free_tcp_port()
        self.console_port = self._unsafe_get_free_tcp_port()

    def grab_minidump(self, minidump_dest):
        """Tries to grab a minidump and copies it to the given destination."""

        # Okay, this is a bit of a hack.
        # We have a seperate process that captures the .dmp file. This process takes
        # some time to complete, once that process completes the .dmp file will be deleted.
        # we have to grab it before it disappears.
        android_home = os.environ.get('ANDROID_HOME',  os.path.join(os.path.expanduser('~'), '.android'))
        breakpad_dmps = os.path.join(android_home, 'breakpad', '*.dmp')
        list_of_files = []
        # Let's wait at most 10 seconds..
        timeout = time.time() + 10
        while not list_of_files and time.time() < timeout:
            logging.info("Waiting for mini dump in %s", breakpad_dmps)
            list_of_files = glob.glob(breakpad_dmps) # * means all if need specific format then *.csv
            time.sleep(0.1)

        if not list_of_files:
            logging.info("Did not find a minidump file.")
            return

        latest_file = max(list_of_files, key=os.path.getctime)
        shutil.copy2(latest_file, minidump_dest)
        logging.info("Copied %s -> %s", latest_file, minidump_dest)

    def get_avd(self):
        """Just pick the first reported avd from the emulator."""
        return next(iter(subprocess.check_output([self.emulator, '-list-avds']).split()), None)


    def is_alive(self):
        """Returns true if the emulator has booted."""
        cmd = ['adb', 'shell', 'getprop', 'sys.boot_completed']
        try:
            return '1' in subprocess.check_output(cmd).decode("utf-8")
        except subprocess.CalledProcessError:
            return False


    def _unsafe_get_free_tcp_port(self):
        """Tries to find a free tcp port, not really safe."""
        # Race conditions totally possible..
        tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp.bind(('', 0))
        _, port = tcp.getsockname()
        tcp.close()
        return port

    def launch(self):
        """Super simplistic, poor man's emulator launcher.

        This will start the emulator with the configured avd, and will wait
        until it has booted up, or until it hits a timeout.
        """
        cmd = [
            self.emulator,
            '-avd',
            self.avd,
            '-detect-image-hang',
            '-skip-adb-auth',
            '-ports',
            '%d,%d' % (self.console_port, self.adb_port)

        ]
        proc = psutil.Popen(cmd)
        max_wait = self.max_boot
        while (proc.status() not in [
            psutil.STATUS_DEAD, psutil.STATUS_ZOMBIE, psutil.STATUS_STOPPED
        ] and max_wait > 0 and not self.is_alive()):
            time.sleep(1)
            max_wait = max_wait - 1
