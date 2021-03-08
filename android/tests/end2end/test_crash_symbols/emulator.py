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
import platform
import glob
import socket
import subprocess
import shutil
import time

from Queue import Queue
from threading import Thread
from absl import logging
from emulator_connection import EmulatorConnection


class Emulator(object):

    def __init__(self, emulator_exe, max_boot, sdk_root=None, avd_home=None, avd=None):
        self.max_boot = max_boot
        self.sdk_root = os.path.abspath(sdk_root or os.environ.get('ANDROID_SDK_ROOT'))
        self.emulator = emulator_exe or os.path.join(self.sdk_root, 'emulator', 'emulator')
        self.avd_home = os.path.abspath(avd_home or os.environ.get('ANDROID_HOME',  os.path.join(
            os.path.expanduser('~'), '.android')))
        self.avd = avd or self.get_avd()
        self.adb_port = self._unsafe_get_free_tcp_port()
        self.console_port = self._unsafe_get_free_tcp_port()
        self.proc = None

    def grab_minidump(self, minidump_dest):
        """Tries to grab a minidump and copies it to the given destination."""

        # Okay, this is a bit of a hack.
        # We have a seperate process that captures the .dmp file. This process takes
        # some time to complete, once that process completes the .dmp file will be deleted.
        # we have to grab it before it disappears.
        android_home = self.avd_home
        breakpad_dmps = os.path.join(android_home, 'breakpad', '*.dmp')
        list_of_files = []
        # Let's wait at most 10 seconds..
        timeout = time.time() + 10
        while not list_of_files and time.time() < timeout:
            logging.info("Waiting for mini dump in %s", breakpad_dmps)
            list_of_files = glob.glob(breakpad_dmps)
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
        """Returns true if the emulator console is active."""
        sock = None
        time.sleep(1)
        try:
            sock = socket.create_connection(('localhost', self.console_port))
            msg = sock.recv(8)
            return 'Android' in msg
        except socket.error, _:
            return False
        finally:
            if sock:
                sock.close()

    def connect_console(self):
        return EmulatorConnection.connect(self.console_port)

    def launch(self, additional_args=None):
        """Super simplistic, poor man's emulator launcher.

        This will start the emulator with the configured avd, and will wait
        until the console is available, or until it hits a timeout.
        """
        cmd = [
            self.emulator,
            '-avd',
            self.avd,
            '-ports',
            '%d,%d' % (self.console_port, self.adb_port)
        ]

        if additional_args:
            cmd += additional_args

        use_shell = (platform.system() == 'Windows')
        local_env = os.environ.copy()
        if self.avd_home:
            # Setup android sdk/avd etc.
            local_env['ANDROID_SDK_HOME'] = self.avd_home
            local_env.pop('ANDROID_AVD_HOME', None)
        if self.sdk_root:
            local_env['ANDROID_SDK_ROOT'] = self.sdk_root

        logging.info("Launching %s with: %s", cmd, local_env)
        self.proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=use_shell,
            env=local_env)

        Thread(target=self._log_std_out, args=[self.proc]).start()

        max_wait = self.max_boot
        logging.info("Waiting for port %d to become available",
                     self.console_port)
        while (not self.proc.poll() and max_wait > 0 and not self.is_alive()):
            time.sleep(1)
            logging.info(
                ".. Waiting for port %d to become available", self.console_port)
            max_wait = max_wait - 1

    def _reader(self, pipe, queue):
        try:
            with pipe:
                for line in iter(pipe.readline, b''):
                    queue.put((pipe, line[:-1]))
        finally:
            queue.put(None)

    def _log_std_out(self, proc):
        """Logs the output of the given process."""
        q = Queue()
        Thread(target=self._reader, args=[proc.stdout, q]).start()
        Thread(target=self._reader, args=[proc.stderr, q]).start()
        for _ in range(2):
            for _, line in iter(q.get, None):
                logging.info(line)

    def _unsafe_get_free_tcp_port(self):
        """Tries to find a free tcp port, not really safe."""
        # Race conditions totally possible..
        tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp.bind(('', 0))
        _, port = tcp.getsockname()
        tcp.close()
        return port
