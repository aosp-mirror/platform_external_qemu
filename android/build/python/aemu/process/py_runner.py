# Copyright 2023 - The Android Open Source Project
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
import logging
import platform
import tempfile
from pathlib import Path

from aemu.process.command import Command


class PyRunner:
    """A utility class that helps to run Python commands and install packages within
    a specified repository."""

    def __init__(self, repo, aosp):
        self.repo = repo
        self.aosp = aosp
        self.python = self._get_python_exe()
        self.env = {}
        if platform.system() == "Windows":
            self._fixup_windows_py3_dll()
            Command(
                [
                    self.python,
                    self.aosp / "external" / "adt-infra" / "devpi" / "get-pip.py",
                    "--no-wheel",
                    "--no-setuptools",
                    "--index-url",
                    f"{repo}",
                ],
            ).run()
            self.py_exe = self.python
        else:
            self.tmp = tempfile.TemporaryDirectory()
            tmpdir = Path(self.tmp.name)
            Command(
                [
                    self.python,
                    "-m",
                    "venv",
                    tmpdir / ".venv",
                ],
            ).run()

            self.py_exe = tmpdir / ".venv" / "bin" / "python"
            self.env["VIRTUAL_ENV"] = str(tmpdir / ".venv")

        self.run(
            ["-m", "pip", "install", "--upgrade", "pip", "--index-url", f"{self.repo}"]
        )

    def _get_python_exe(self):
        """Returns the python interpreter we ship with aosp.

        The samples are guaranteed to work against this version.
        """
        os_name = platform.system().lower()
        python_dir = self.aosp / "prebuilts" / "python" / f"{os_name}-x86"
        if os_name != "windows":
            return python_dir / "bin" / "python3"
        else:
            return python_dir / "python.exe"

    def run(self, args, env={}, cwd=os.getcwd()):
        """This method runs a Python command with the specified arguments, environment variables, and timeout.

        Args:
            args ([str]): Set of arguments to give to python interpreter
            env (dict[str, str]): Optional environment to use
            timeout (int): Optional timeout in seconds to use.
        """
        emu_env = self.env.copy()
        emu_env.update(env)
        logging.info("Using %s from %s", emu_env, self.env)
        Command([self.py_exe] + args,).with_environment(
            emu_env
        ).in_directory(cwd).run()

    def _fixup_windows_py3_dll(self):
        # Fixup incorrect dll in windows see b/265843618.
        python_dir = self.aosp / "prebuilts" / "python" / "windows-x86"
        py310dll = python_dir / "python310.dll"
        py3dll = python_dir / "Python3.dll"
        assert (
            py310dll
        ).exists(), (
            "python310.dll does not exist, did you upgrade the python interpreter?"
        )
        if not py3dll.exists():
            py3dll.symlink_to(py310dll)

    def pip_install(self, packages):
        """installs the specified packages using pip"

        Args:
            packages ([str]): The set of packages to install
        """
        if platform.system() == "Windows":
            self.run(
                [
                    "-m",
                    "pip",
                    "install",
                    "--user",
                    "--upgrade",
                    "--index-url",
                    f"{self.repo}",
                ]
                + packages,
            )
        else:
            self.run(
                ["-m", "pip", "install", "--upgrade", "--index-url", f"{self.repo}"]
                + packages,
                env=self.env,
            )
