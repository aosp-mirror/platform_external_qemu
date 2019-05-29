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

import os
import platform
import shutil
import subprocess
import sys
import tempfile
from Queue import Queue
from threading import Thread
from distutils.spawn import find_executable


from absl import flags
from absl import logging
from absl import app

FLAGS = flags.FLAGS
# Beware! This depends on this file NOT being installed!
flags.DEFINE_string('aosp_root', os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(
    __file__)), '..', '..', '..', '..')), 'The aosp directory')


class TemporaryDirectory(object):
    """Creates a temporary directory that will be deleted when moving out of scope"""

    def __enter__(self):
        self.name = tempfile.mkdtemp()
        self.cwd = os.getcwd()
        os.chdir(self.name)
        return self.name

    def __exit__(self, exc_type, exc_value, traceback):
        os.chdir(self.cwd)
        if platform.system() == 'Windows':
            run(['rmdir', '/s', '/q', self.name])
        else:
            shutil.rmtree(self.name)


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


def run(cmd, cwd=None):
    if cwd:
        cwd = os.path.abspath(cwd)

    # We need to use a shell under windows, to set environment parameters.
    # otherwise we don't, since we will experience cmake invocation errors.
    use_shell = (platform.system() == 'Windows')

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=cwd,
        shell=use_shell
    )

    log_std_out(proc)
    proc.wait()
    if proc.returncode != 0:
        raise Exception('Failed to run %s - %s' %
                        (' '.join(cmd), proc.returncode))


def recursive_iglob(rootdir, names):
    '''Recursively glob the rootdir for any file that matches the files we are looking for.'''
    for root, _, filenames in os.walk(rootdir):
        for filename in filenames:
            fname = os.path.join(root, filename)
            if filename in names:
                yield fname


def main(argv=None):
    if platform.system() != 'Windows':
        logging.fatal("Currently only works for Windows")
        sys.exit(1)

    with TemporaryDirectory() as tmp:
        swift = os.path.join(tmp, 'swiftshader')
        build_dir = os.path.join(swift, 'build')

        git = find_executable('git')
        run([git, 'clone', 'https://github.com/google/swiftshader.git', '--progress'], tmp)
        # Note: emulator branch is not working with msvc build!
        # run([git, 'checkout',  'android-emulator-current-release'], swift)

        # Let's build it.
        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        cmd = ["cmake", ".."]
        if platform.system() == "Windows":
            cmd += ["-DCMAKE_GENERATOR_PLATFORM=x64", "-Thost=x64"]

        run(cmd, build_dir)
        cmd = ["cmake",  "--build",  "."]
        run(cmd, build_dir)

        # Now find the files we want:
        copyset = ['libEGL.dll', 'libEGL.pdb', 'libEGL.lib',
                   'libGLES_CM.dll', 'libGLES_CM.pdb', 'libGLES_CM.lib',
                   'libGLESv2.dll', 'libGLESv2.pdb', 'libGLESv2.lib']
        dest = 'windows-msvc-x86_64'

        to_copy = set(recursive_iglob(build_dir, copyset))
        for fn in to_copy:
            dst = os.path.join(FLAGS.aosp_root, 'prebuilts',
                               'android-emulator-build', 'common', 'swiftshader', dest, 'lib')
            if not os.path.exists(dst):
                os.makedirs(dst)

            logging.info("Copy %s -> %s", fn, dst)
            shutil.copy2(fn, dst)


if __name__ == '__main__':
    app.run(main)
