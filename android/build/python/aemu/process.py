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
import platform
from Queue import Queue
import subprocess
from threading import Thread

from aemu.definitions import get_qemu_root, get_visual_studio
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
            try:
                logging.info(line)
            except IOError:
                # We sometimes see IOError, errno = 0 on windows.
                pass



_CACHED_ENV = None

def get_system_env():
    # Configure the paths for the various development environments
    global _CACHED_ENV
    if _CACHED_ENV != None:
        return _CACHED_ENV

    local_env = os.environ.copy()
    if platform.system() == 'Windows':
        env_lines = subprocess.check_output(["cmd", "/c", get_visual_studio(), "&&",  "set"]).splitlines()
        for env_line in env_lines:
            if '=' in env_line:
                env = env_line.split('=')
                # Variables in windows are case insensitive, but not in python dict!
                local_env[env[0].upper()] = env[1]
    else:
        local_env['PATH'] = os.path.join(get_qemu_root(), 'android', 'third_party', 'chromium', 'depot_tools') + os.pathsep + local_env['PATH']

    _CACHED_ENV = local_env
    return local_env

def run(cmd, cwd=None, extra_env=None):
    if cwd:
        cwd = os.path.abspath(cwd)

    # We need to use a shell under windows, to set environment parameters.
    # otherwise we don't, since we will experience cmake invocation errors.
    use_shell = (platform.system() == 'Windows')
    local_env = get_system_env()
    if extra_env:
        local_env.update(extra_env)

    logging.info('Running: %s in %s', ' '.join(cmd), cwd)
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=cwd,
        shell=use_shell, # Needed on windows, otherwise the environment won't propagate.
        env=local_env)

    log_std_out(proc)
    proc.wait()
    if proc.returncode != 0:
        raise Exception('Failed to run %s - %s' %
                        (' '.join(cmd), proc.returncode))
