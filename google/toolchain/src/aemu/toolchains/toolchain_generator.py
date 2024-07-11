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
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

from aemu.process.bazel import Bazel


class ToolchainGenerator:
    PKGCFG_DIR = "pc-config"

    def __init__(self, aosp: Path, dest: Path, prefix: str) -> None:
        self.aosp = aosp.absolute()
        self.bazel = Bazel(self.aosp, dest)

        toolchain_json = self.aosp / "build" / "bazel" / "rules" / "toolchains.json"
        with open(
            toolchain_json,
            encoding="utf-8",
        ) as f:
            self.versions = json.load(f)
        self.dest: Path = dest
        self.prefix = prefix
        self.toolchain_map = {}
        self.dest.mkdir(exist_ok=True, parents=True)
        self.ccache = shutil.which("sccache") or shutil.which("ccache")
        self.py_exe = Path(sys.executable).absolute()

        # Create pkgconfig directory.
        self.pkgconfig_directory = self.dest / ToolchainGenerator.PKGCFG_DIR
        self.pkgconfig_directory.mkdir(parents=True, exist_ok=True)

    def version(self):
        return self.versions.get("clang", "clang-stable")

    def rust_version(self):
        return self.versions.get("rust", "1.78.0")

    def cc_version(self):
        logging.info("Using clang: %s", self.clang())
        result = subprocess.check_output(
            [str(self.clang() / "bin" / "clang"), "-v"],
            encoding="utf-8",
            stderr=subprocess.STDOUT,
        )
        version = result.splitlines()[0]
        match = re.match(r".*clang version (\d+).(\d+).(\d).*", version)
        if match:
            return match[1]

        raise ValueError(f"Could not find version string in {version}")

    def clang(self) -> Path:
        return (
            self.aosp
            / "prebuilts"
            / "clang"
            / "host"
            / f"{self.host()}-x86"
            / self.version()
        )

    def cmake(self) -> Path:
        cmake = (
            self.aosp / "prebuilts" / "cmake" / f"{self.host()}-x86" / "bin" / "cmake"
        )
        return (
            f"{cmake} ",
            "",
        )

    def host(self) -> str:
        return platform.system().lower()

    def nm(self) -> (str, str):
        return self.clang() / "bin" / "llvm-nm", ""

    def ar(self) -> (str, str):
        return self.clang() / "bin" / "llvm-ar", ""

    def objdump(self) -> (str, str):
        return self.clang() / "bin" / "llvm-objdump", ""

    def strings(self) -> (str, str):
        return self.clang() / "bin" / "llvm-strings", ""

    def ranlib(self) -> (str, str):
        return self.clang() / "bin" / "llvm-ranlib", ""

    def cxx(self) -> (str, str):
        return self.clang() / "bin" / "clang++", ""

    def cc(self) -> (str, str):
        return self.clang() / "bin" / "clang", ""

    def lld(self) -> (str, str):
        return self.clang() / "bin" / "lld", ""

    def nm(self) -> (str, str):
        return self.clang() / "bin" / "llvm-nm", ""

    def ninja(self) -> (str, str):
        return (
            self.aosp
            / "prebuilts"
            / "build-tools"
            / f"{self.host()}-x86"
            / "bin"
            / "ninja",
            "",
        )

    def rust_flags(self) -> [str]:
        return []

    def rustc(self) -> (str, str):
        return (
            self.aosp
            / "prebuilts"
            / "rust"
            / f"{self.host()}-x86"
            / self.rust_version()
            / "bin"
            / "rustc",
            "",
        )

    def cargo(self) -> (str, str):
        rustc_bin = (
            self.aosp
            / "prebuilts"
            / "rust"
            / f"{self.host()}-x86"
            / self.rust_version()
            / "bin"
            / "rustc"
        )
        cargo_bin = (
            self.aosp
            / "prebuilts"
            / "rust"
            / f"{self.host()}-x86"
            / self.rust_version()
            / "bin"
            / "cargo"
        )

        script = f"CARGO_BUILD_RUSTC={rustc_bin}\n" f"{cargo_bin}"
        return script, ""

    def meson(self) -> (str, str):
        meson_py = self.aosp / "external" / "meson" / "meson.py"
        exe = f"{self.py_exe} {meson_py} "
        return exe, ""

    def strip(self):
        return "", ""

    def pkg_config(self):
        # Build pkg-config from source.
        self.bazel.build_target("//external/pkg-config:pkg-config")
        return (
            f"PKG_CONFIG_PATH={self.pkgconfig_directory} "
            f"{self.bazel.info['bazel-bin']}/external/pkg-config/pkg-config",
            "",
        )

    def gen_script(self, name, exe: Path, cmd_generator_fn):
        current_file = Path(__file__).resolve()
        self.toolchain_map[name] = exe.absolute().as_posix()

        logging.info("Generating %s", exe)

        rem = "#"
        run, extra = cmd_generator_fn()
        params = '"$@"'

        script = f"""#!/bin/sh
{rem} Auto-generated by {current_file}, DO NOT EDIT!!
{run} {params} {extra}
"""
        with open(exe, "w", encoding="utf-8") as f:
            f.write(script)

        exe.chmod(0o755)

    def _get_toolchain_config(self):
        result = f"# Auto generated by Android Meson Generator - do not modify\n"
        result += """[properties]
[built-in options]
c_args = []
cpp_args = []
objc_args = []
c_link_args = []
cpp_link_args = []
[binaries]
"""
        for name, dest in self.toolchain_map.items():
            result += f"{name} = '{dest}'\n"

        cc = self.toolchain_map["cc"]
        result += f"c = '{cc}'\n"
        return result

    def write_toolchain_config(self):
        """Writes a toolchain configuration that can be consumed by meson."""
        with open(self.dest / "aosp-cl.ini", "w", encoding="utf-8") as f:
            f.write(self._get_toolchain_config())

    def link_dirs(self):
        """Setup links to libc++.so etc.."""
        clang_lib = self.clang() / "lib"
        if not clang_lib.exists():
            logging.warning("The clang lib directory: %s, does not exist.", clang_lib)
            return

        (self.dest / "lib").symlink_to(clang_lib)

    def gen_toolchain(self):
        cmds = {
            "nm": self.nm,
            "ar": self.ar,
            "c++": self.cxx,
            "cc": self.cc,
            "g++": self.cxx,
            "gcc": self.cc,
            "lld": self.lld,
            "ld": self.lld,
            "objdump": self.objdump,
            "strings": self.strings,
            "ranlib": self.ranlib,
            "meson": self.meson,
            "ninja": self.ninja,
            "pkg-config": self.pkg_config,
            "strip": self.strip,
            "cmake": self.cmake,
            "cargo": self.cargo,
            "rustc": self.rustc,
        }
        for cmd, fn in cmds.items():
            self.gen_script(cmd, self.dest / f"{self.prefix}{cmd}", fn)

        self.link_dirs()
