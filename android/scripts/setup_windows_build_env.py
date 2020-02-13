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
import argparse
import ctypes
import logging
import os
import shutil
import subprocess
import sys
import urllib3
import webbrowser
import threading
from queue import Queue

if sys.version_info[0] == 2:
    raise Exception("You will need to install Python3 to run this script")

REPO_STORAGE_URL = "http://storage.googleapis.com/git-repo-downloads/repo"

print(
    "This is a script that automates the steps in https://android.googlesource.com/platform/external/qemu/+/emu-master-dev/android/docs/WINDOWS-DEV.md"
)


def _reader(pipe, queue):
    try:
        with pipe:
            for line in iter(pipe.readline, b""):
                queue.put((pipe, line[:-1]))
    finally:
        queue.put(None)


def log_std_out(proc):
    """Logs the output of the given process."""
    q = Queue()
    threading.Thread(target=_reader, args=[proc.stdout, q]).start()
    threading.Thread(target=_reader, args=[proc.stderr, q]).start()
    for _ in range(2):
        for _, line in iter(q.get, None):
            try:
                logging.info(line)
            except IOError:
                # We sometimes see IOError, errno = 0 on windows.
                pass


def run(cmd, cwd=None):
    if cwd:
        cwd = os.path.abspath(cwd)

    cmd = [str(c) for c in cmd]

    logging.info("Running: %s in %s", " ".join(cmd), cwd)
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=cwd,
        env={},
        shell=True
    )

    log_std_out(proc)
    proc.wait()
    if proc.returncode != 0:
        raise Exception("Failed to run %s - %s" % (" ".join(cmd), proc.returncode))


def config_git():
    subprocess.check_call(["git", "config", "--system", "core.symlinks", "true"])
    subprocess.check_call(["git", "config", "--global", "core.symlinks", "true"])


def setup_repo(repo_bin_dir):
    repo_py = os.path.join(repo_bin_dir, "repo")
    repo_cmd = os.path.join(repo_bin_dir, "repo.cmd")
    http = urllib3.PoolManager()
    r = http.request('GET', REPO_STORAGE_URL, preload_content=False)

    with open(repo_py, 'wb') as out:
        while True:
            data = r.read(4096)
            if not data:
                break
            out.write(data)
    
    with open(repo_cmd, 'wb') as repo:
        repo.writelines([b'@echo off\r\n', b'@call python %~dp0repo %*'])

   
def initialize_repo(repo_bin_dir, src_dir):
    repo_cmd = os.path.join(repo_bin_dir, "repo.cmd")
    if not os.path.exists(src_dir):
        os.makedirs(src_dir)
    run(
        [
            repo_cmd,
            "init",
            "-u",
            "https://android.googlesource.com/platform/manifest",
            "-b",
            "emu-master-dev",
        ],
        src_dir,
    )


def sync_repo(repo_bin_dir, src_dir):
    repo_cmd = os.path.join(repo_bin_dir, "repo.cmd")
    run([repo_cmd, "sync", "-f", "--no-tags", "--optimized-fetch", "--prune"], src_dir)


def get_git_cookie():
    webbrowser.open_new("https://www.googlesource.com/new-password")


def main():
    parser = argparse.ArgumentParser(
        description="Create a windows development environment.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--bin",
        default=os.path.join(os.environ["USERPROFILE"], "bin"),
        help="Binary directory where to place the repo command. Should be on your path.",
    )
    parser.add_argument(
        "src",
        default="C:\\development\\emu-master-dev",
        help="Destination directory where to checkout the source",
    )

    args = parser.parse_args()

    if ctypes.windll.shell32.IsUserAnAdmin() != 1:
        raise Exception("You need to run this with Administrator priveleges")

    logging.basicConfig(level=logging.DEBUG)
    print("Installing repo in {} and source in {}".format(args.bin, args.src))
    input("Press Enter to continue...")

    config_git()
    setup_repo(args.bin)
    initialize_repo(args.bin, args.src)
    sync_repo(args.bin, args.src)
    get_git_cookie()

    print("Completed repo sync, follow the browser steps to get a git cookie")


if __name__ == "__main__":
    main()
