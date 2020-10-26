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
import logging
import os
import platform
import subprocess
import sys
from threading import Thread

from aemu.definitions import get_qemu_root, get_visual_studio

if sys.version_info[0] == 3:
    from queue import Queue
else:
    from Queue import Queue


def _reader(pipe, logfn):
    try:
        with pipe:
            for line in iter(pipe.readline, b""):
                logfn(line[:-1].decode("utf-8").strip())
    finally:
        pass


def _log_proc(proc):
    """Logs the output of the given process."""
    q = Queue()
    for args in [[proc.stdout, logging.info], [proc.stderr, logging.error]]:
        Thread(target=_reader, args=args).start()

    return q


_CACHED_ENV = None


def get_system_env():
    # Configure the paths for the various development environments
    global _CACHED_ENV
    if _CACHED_ENV != None:
        return _CACHED_ENV

    local_env = os.environ.copy()
    if platform.system() == "Windows":
        vs = get_visual_studio()
        env_lines = subprocess.check_output([vs, "&&", "set"]).splitlines()
        for env_line in env_lines:
            line = str(env_line)
            if "=" in line:
                env = line.split("=")
                # Variables in windows are case insensitive, but not in python dict!
                local_env[env[0].upper()] = env[1]
    else:
        local_env["PATH"] = (
            os.path.join(
                get_qemu_root(), "android", "third_party", "chromium", "depot_tools"
            )
            + os.pathsep
            + local_env["PATH"]
        )

    _CACHED_ENV = local_env
    return local_env


def run(cmd, cwd=None, extra_env=None):
    if cwd:
        cwd = os.path.abspath(cwd)

    cmd = [str(c) for c in cmd]

    # We need to use a shell under windows, to set environment parameters.
    # otherwise we don't, since we will experience cmake invocation errors.
    use_shell = platform.system() == "Windows"
    local_env = get_system_env()
    if extra_env:
        local_env.update(extra_env)

    logging.info("Running: %s in %s", " ".join(cmd), cwd)
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=cwd,
        shell=use_shell,  # Needed on windows, otherwise the environment won't propagate.
        env=local_env,
    )

    _log_proc(proc)
    proc.wait()
    if proc.returncode != 0:
        raise Exception("Failed to run %s - %s" % (" ".join(cmd), proc.returncode))
