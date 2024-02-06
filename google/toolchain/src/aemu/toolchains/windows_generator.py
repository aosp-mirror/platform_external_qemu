# -*- coding: utf-8 -*-
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
import json
import logging
import os
import shutil
import subprocess
from pathlib import Path

from aemu.toolchains.toolchain_generator import ToolchainGenerator


class VisualStudioNotFoundException(Exception):
    pass


class VisualStudioMissingVarException(Exception):
    pass


class VisualStudioNativeWorkloadNotFoundException(Exception):
    pass


class WindowsToWindowsGenerator(ToolchainGenerator):
    COMPAT_ARCHIVE = "//external/qemu/google/compat/windows:compat"

    def __init__(self, aosp, dest, prefix):
        super().__init__(aosp, dest, prefix)
        self.env = {}
        for key in os.environ:
            self.env[key.upper()] = os.environ[key]
        logging.info("Starting environment: %s", self.env)

        self.compat_path = (
            (
                self.aosp
                / "external"
                / "qemu"
                / "google"
                / "compat"
                / "windows"
                / "includes"
            )
            .absolute()
            .as_posix()
        )

    def initialize(self):
        if hasattr(self, "initialized"):
            return
        self._load_visual_studio_env()
        logging.info("Loaded visual studio env: %s", self.env)
        self.bazel.build_target(self.COMPAT_ARCHIVE)
        self.initialized = True

    def _load_visual_studio_env(self):
        vs = self._visual_studio()
        logging.info("Loading environment from %s", vs)
        env_lines = subprocess.check_output(
            [vs, "&&", "set"], encoding="utf-8"
        ).splitlines()
        for line in env_lines:
            if "=" in line:
                key, val = line.split("=", 1)
                # Variables in windows are case insensitive, but not in python dict!
                self.env[key.upper()] = val

        if "VSINSTALLDIR" not in self.env:
            raise VisualStudioMissingVarException("Missing VSINSTALLDIR in environment")

        if "VCTOOLSINSTALLDIR" not in self.env:
            raise VisualStudioMissingVarException(
                "Missing VCTOOLSINSTALLDIR in environment"
            )

    def _get_toolchain_config(self):
        result = super()._get_toolchain_config()
        result += "\n"
        result += "[host_machine]\n"
        result += "system = 'windows'\n"
        result += "cpu_family = 'x86_64'\n"
        result += "cpu = 'x86_64'\n"
        result += "endian = 'little'\n"
        return result

    def _visual_studio(self):
        """Finds the visual studio installation

        Raises:
            VisualStudioNotFoundException: When visual studio was not found
            VisualStudioNativeWorkloadNotFoundException: When the native workload was not found

        Returns:
            _type_: _description_
        """
        prgrfiles = Path(os.getenv("ProgramFiles(x86)", "C:\Program Files (x86)"))
        res = subprocess.check_output(
            [
                str(
                    prgrfiles / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
                ),
                "-requires",
                "Microsoft.VisualStudio.Workload.NativeDesktop",
                "-sort",
                "-format",
                "json",
                "-utf8",
            ]
        )
        vsresult = json.loads(res)
        if len(vsresult) == 0:
            raise VisualStudioNativeWorkloadNotFoundException(
                f"No visual studio with the native desktop load available in {res}"
            )

        for install in vsresult:
            logging.debug("Considering %s", install["displayName"])
            candidates = list(Path(install["installationPath"]).glob("**/vcvars64.bat"))

            if len(candidates) > 0:
                return candidates[0].absolute()

        # Oh oh, no visual studio..
        raise VisualStudioNotFoundException(
            f"Unable to detect a visual studio installation with the native desktop workload from {res}."
        )

    def gen_script(self, name, location: Path, cmd_generator_fn):
        # Generate windows version of the scripts.
        current_file = Path(__file__).resolve()
        exe = location.with_suffix(".cmd")
        logging.info("Generating %s", exe)
        self.toolchain_map[name] = exe.absolute().as_posix()

        rem = "rem "
        run, extra = cmd_generator_fn()
        params = "%*"

        script = f"""@echo off
{rem} Auto-generated by {current_file}, DO NOT EDIT!!
{run} {params} {extra}
"""
        with open(exe, "w", encoding="utf-8") as f:
            f.write(script)

        exe.chmod(0o755)

    def gen_toolchain(self):
        super().gen_toolchain()

        # Generate the resource compilers.
        self.gen_script("windres", self.dest / "rc", self.windres)
        self.gen_script("windmc", self.dest / "rc", self.windres)

    def ninja(self):
        # We do not have ninja for windows in AOSP, so we will pick up the one that ships
        # with visual studio.
        ninja = shutil.which("ninja", path=self.env["PATH"])
        if not ninja:
            raise FileNotFoundError(
                f"Ninja was not found on the path: {self.env['PATH']}, please install visual studio with Ninja."
            )

        return f'"{ninja}"', ""

    def cmake(self) -> Path:
        vs = self._visual_studio()
        cmake = (
            self.aosp / "prebuilts" / "cmake" / f"{self.host()}-x86" / "bin" / "cmake"
        )
        # We make sure that the vs variables are loaded before launching cmake
        # This will enable cmake to derive the visual studio compiler toolchain.
        return (
            f'call "{vs}"\n' f"{cmake}",
            "",
        )

    def pkg_config(self):
        # Build pkg-config from source.
        self.bazel.build_target("//external/pkg-config:pkg-config")
        pkg_exe = (
            Path(self.bazel.info["bazel-bin"])
            / "external"
            / "pkg-config"
            / "pkg-config.exe"
        )
        pkg_path = self.pkgconfig_directory.as_posix()
        return (
            f"set PKG_CONFIG_PATH={pkg_path}\n" f"{pkg_exe}",
            "",
        )

    def cc(self, clang="clang"):
        self.initialize()
        compat_lib_dir = self.bazel.get_archive(self.COMPAT_ARCHIVE).parent

        cache = f"{self.ccache}" if self.ccache else ""
        cl = self.clang() / "bin" / clang
        flags = (
            "-Wno-constant-conversion "
            "-Wno-macro-redefined "
            "-Wno-invalid-noreturn "
            "-Wno-bitfield-constant-conversion "
            "-Wno-int-to-void-pointer-cast "
            "-Wno-unused-command-line-argument "
            "-Wno-undef "
            "-Wno-microsoft-enum-forward-reference "
            "-Wno-microsoft-include "
            "-Wno-deprecated-declarations "
            f"-L{compat_lib_dir} -lcompat"
        )

        return (
            f"{cache} {cl} -I{self.compat_path} -march=native -target x86_64-pc-windows-msvc -fms-extensions",
            f"{flags}",
        )

    def cxx(self):
        return self.cc("clang++")

    def windres(self):
        winsdk_rc_bin = (
            Path(self.env["WINDOWSSDKDIR"])
            / "bin"
            / self.env["WINDOWSSDKLIBVERSION"]
            / "x64"
            / "rc.exe"
        )
        return f'"{winsdk_rc_bin}"', ""

    def lld(self):
        winsdk_lib = (
            Path(self.env["WINDOWSSDKDIR"])
            / "Lib"
            / self.env["WINDOWSSDKLIBVERSION"]
            / "um"
            / "x64"
        )
        lld = self.clang() / "bin" / "lld-link"
        return f'{lld} /libpath:"{winsdk_lib}"', ""
