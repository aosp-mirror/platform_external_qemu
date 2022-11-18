# Copyright 2022 - The Android Open Source Project
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
import platform
from distutils.spawn import find_executable
from pathlib import Path
from typing import List

from aemu.platform.toolchains import Toolchain
from aemu.process.command import Command
from aemu.process.environment import get_default_environment
from aemu.tasks.build_task import BuildTask


class ConfigureTask(BuildTask):
    """Runs the CMake Ninja generator."""

    CRASH = {
        "prod": [
            "-DOPTION_CRASHUPLOAD=PROD",
            "-DBREAKPAD_API_URL=https://prod-crashsymbolcollector-pa.googleapis.com",
        ],
        "staging": [
            "-DOPTION_CRASHUPLOAD=STAGING",
            "-DBREAKPAD_API_URL=https://staging-crashsymbolcollector-pa.googleapis.com",
        ],
        "none": ["-DOPTION_CRASHUPLOAD=NONE"],
    }
    BUILDCONFIG = {
        "debug": ["-DCMAKE_BUILD_TYPE=Debug"],
        "release": ["-DCMAKE_BUILD_TYPE=Release"],
    }

    def __init__(
        self,
        aosp: Path,
        target: str,
        destination: Path,
        crash: str,
        configuration: str,
        build_number: str,
        webengine: bool,
        gfxstream: bool,
        gfxstream_only: bool,
        sanitizer: List[str],
        options: List[str],
        ccache: str,
    ):
        super().__init__()
        self.toolchain = Toolchain(aosp, target)
        cmake = find_executable(
            "cmake",
            path=str(
                aosp
                / "prebuilts"
                / "cmake"
                / f"{platform.system().lower()}-x86"
                / "bin"
            ),
        )
        self.cmake_cmd = [
            cmake,
            f"-B{destination}",
            "-G Ninja",
        ]
        self.cmake_cmd += self.CRASH[crash]
        self.cmake_cmd += self.BUILDCONFIG[configuration]

        self.add_option(
            "CMAKE_TOOLCHAIN_FILE", self.toolchain.cmake_toolchain()
        ).add_option("OPTION_SDK_TOOLS_BUILD_NUMBER", build_number)
        if webengine:
            self.with_webengine()

        if gfxstream:
            self.with_gfxstream()

        if gfxstream_only:
            self.with_gfxstream_only()

        if self._find_ccache(aosp, ccache):
            self.with_ccache(self._find_ccache(aosp, ccache))

        self.with_options(options)

        if sanitizer:
            self.add_option("OPTION_ASAN", ",".join(sanitizer))

        self.cmake_cmd.append(aosp / "external" / "qemu")
        self._add_sdk_revision(aosp)
        self.env = get_default_environment(aosp)

    def add_option(self, key: str, val: str):
        self.cmake_cmd += [f"-D{key}={val}"]
        return self

    def _add_sdk_revision(self, aosp: Path) -> str:
        revision = None
        with open(
            aosp / "external" / "qemu" / "source.properties", "r", encoding="utf-8"
        ) as props:
            for line in props.readlines():
                key, value = line.split("=")
                if key == "Pkg.Revision":
                    revision = value.strip()
        self.add_option("OPTION_SDK_TOOLS_REVISION", revision)

    def _find_ccache(self, aosp: Path, cache: str()):
        """Locates the cache executable as a posix path"""
        if cache == "none" or cache is None:
            return None

        if cache != "auto":
            return Path(cache)

        host = platform.system().lower()
        if host != "windows":
            # prefer ccache for local builds, it is slightly faster
            # but does not work on windows.
            ccache = find_executable("ccache")
            if ccache:
                return Path(ccache)

        # Our build bots use sccache, so we will def. have it
        search_dir = (
            aosp
            / "prebuilts"
            / "android-emulator-build"
            / "common"
            / "sccache"
            / f"{host}-x86_64"
        )
        return Path(find_executable("sccache", search_dir))

    def with_ccache(self, ccache: str):
        return self.add_option("OPTION_CCACHE", ccache)

    def with_webengine(self):
        return self.add_option("QTWEBENGINE", True)

    def with_gfxstream(self):
        return self.add_option("GFXSTREAM", True)

    def with_gfxstream_only(self):
        return self.add_option("GFXSTREAM_ONLY", True)

    def with_options(self, raw_options):
        if raw_options:
            self.cmake_cmd += [f"-D{x}" for x in raw_options]
        return self

    def with_asan(self, sanitizer: [str]):
        return self.add_option("OPTION_ASAN", ",".join(sanitizer))

    def do_run(self):
        logging.info(
            "Configure target: %s, %s compilation",
            self.toolchain.target,
            "cross" if self.toolchain.is_crosscompile() else "native",
        )
        Command(self.cmake_cmd).with_environment(self.env).run()
