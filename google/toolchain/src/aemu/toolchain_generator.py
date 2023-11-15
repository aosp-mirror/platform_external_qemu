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
import subprocess
from pathlib import Path
from collections import namedtuple


AOSP_ROOT = Path(__file__).resolve().parents[6]
BazelInfo = namedtuple("BazelInfo", ["exe", "output_base", "bazel_out", "workspace"])


class ToolchainGenerator:
    TOOLCHAIN_JSON = AOSP_ROOT / "build" / "bazel" / "rules" / "toolchains.json"
    CLANG_DIR = AOSP_ROOT / "prebuilts" / "clang" / "host"
    BAZEL_DIR = AOSP_ROOT / "prebuilts" / "bazel"

    def __init__(self, dest: Path, prefix: str) -> None:
        with open(
            self.TOOLCHAIN_JSON,
            encoding="utf-8",
        ) as f:
            self.versions = json.load(f)
        self.compiler_version = self.cc_version()
        self.dest: Path = dest
        self.dest.mkdir(exist_ok=True, parents=True)
        self.prefix = prefix
        self.bazel = self.load_bazel_info()

    def load_bazel_info(self):
        exe = self.BAZEL_DIR / f"{self.host()}-x86_64" / "bazel"
        output_base = self.run([exe, "info", "output_base"], cwd=AOSP_ROOT).strip()
        bazel_out = self.run([exe, "info", "output_path"], cwd=AOSP_ROOT).strip()
        workspace = self.run([exe, "info", "workspace"], cwd=AOSP_ROOT).strip()
        info = BazelInfo(exe, output_base, bazel_out, workspace)
        logging.debug("Using bazel config: %s", info)
        return info

    def version(self):
        return self.versions.get("clang", "clang-stable")

    def run(self, cmd, cwd: Path = None) -> str:
        """Runs the given command, make sure to use absolute paths!"""
        cmd = [str(x) for x in cmd]
        logging.info("Run: %s", " ".join(cmd))
        return subprocess.check_output(
            cmd, encoding="utf-8", cwd=cwd, stderr=subprocess.DEVNULL, text=True
        )

    def build_target(self, bazel_target: str):
        self.run([self.bazel.exe, "build", bazel_target])

    def cc_version(self):
        logging.info("Using clang: %s", self.clang())
        result = subprocess.check_output(
            [self.clang() / "bin" / "clang", "-v"],
            encoding="utf-8",
            stderr=subprocess.STDOUT,
        )
        version = result.splitlines()[0]
        match = re.match(r".*clang version (\d+.\d+.\d).*", version)
        if match:
            return match[1]
        raise ValueError(f"Could not find version string in {version}")

    def clang(self) -> Path:
        return self.CLANG_DIR / f"{self.host()}-x86" / self.version()

    def host(self):
        return platform.system().lower()

    def nm(self):
        return self.clang() / "bin" / "llvm-nm", ""

    def ar(self):
        return self.clang() / "bin" / "llvm-ar", ""

    def objdump(self):
        return self.clang() / "bin" / "llvm-objdump", ""

    def strings(self):
        return self.clang() / "bin" / "llvm-strings", ""

    def ranlib(self):
        return self.clang() / "bin" / "llvm-ranlib", ""

    def cxx(self):
        return self.clang() / "bin" / "clang++", ""

    def cc(self):
        return self.clang() / "bin" / "clang", ""

    def meson(self):
        self.run([self.bazel.exe, "build", "@meson//:meson"])
        exe = self.run(
            [
                self.bazel.exe,
                "cquery",
                "--output=starlark",
                "--starlark:expr=target.files_to_run.executable.path",
                "@meson//:meson",
            ]
        ).replace("bazel-out", self.bazel.bazel_out)
        return exe, ""

    def gen_script(self, location: Path, cmd_generator_fn):
        current_file = Path(__file__).resolve()
        rem = "#" if not platform.system() == "Windows" else "rem"
        run, extra = cmd_generator_fn()
        params = '"$@"' if not platform.system() == "Windows" else "%*"
        exe = (
            location
            if not platform.system() == "Windows"
            else location.with_suffix(".cmd")
        )

        script = f"""#!/bin/sh
{rem} Auto-generated by {current_file}, DO NOT EDIT!!
{run} {params} {extra}
"""
        logging.info("Generating %s", exe)
        with open(exe, "w", encoding="utf-8") as f:
            f.write(script)

        exe.chmod(0o755)

    def gen_toolchain(self):
        cmds = {
            "nm": self.nm,
            "ar": self.ar,
            "c++": self.cxx,
            "cc": self.cc,
            "objdump": self.objdump,
            "strings": self.strings,
            "ranlib": self.ranlib,
            "meson": self.meson,
        }
        for cmd, fn in cmds.items():
            self.gen_script(self.dest / f"{self.prefix}{cmd}", fn)

        # Create pkgconfig directory.
        pkgconfig_directory = self.dest / "pkgconfig"
        pkgconfig_directory.mkdir(parents=True, exist_ok=True)
