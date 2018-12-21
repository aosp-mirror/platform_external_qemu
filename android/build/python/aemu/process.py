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
import os
from Queue import Queue
import subprocess
from threading import Thread

from aemu.definitions import get_qemu_root
from absl import logging


def _reader(pipe, queue):
    try:
        with pipe:
            for line in iter(pipe.readline, b''):
                queue.put((pipe, line[:-1]))
    finally:
        queue.put(None)


def log_std_out(proc):
    """Logs the output of the given process."""
    q = Queue()
    Thread(target=_reader, args=[proc.stdout, q]).start()
    Thread(target=_reader, args=[proc.stderr, q]).start()
    for _ in range(2):
        for _, line in iter(q.get, None):
            logging.info(line)


def run(cmd, cwd=None):
    if cwd:
        cwd = os.path.abspath(cwd)

    # Make sure we have the depot_tools on the path
    local_env = os.environ.copy()
    local_env['PATH'] = os.path.join(get_qemu_root(), 'android', 'third_party', 'chromium', 'depot_tools') + os.pathsep + local_env['PATH']

    logging.info('Running: %s in %s', ' '.join(cmd), cwd)
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=cwd,
        env=local_env)

    log_std_out(proc)
    proc.wait()
    if proc.returncode != 0:
        raise Exception('Failed to run %s - %s' %
                        (' '.join(cmd), proc.returncode))
