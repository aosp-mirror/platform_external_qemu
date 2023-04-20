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
import os
import platform
import shutil
import sys
from pathlib import Path
from typing import List

from aemu.platform.toolchains import Toolchain
from aemu.process.command import Command, CommandFailedException
from aemu.process.environment import get_default_environment
from aemu.tasks.build_task import BuildTask
from aemu.util.simple_feature_parser import FeatureParser


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
        thread_safety: bool,
        dist: str,
        features: List[str],
    ):
        super().__init__()
        self.toolchain = Toolchain(aosp, target)

        cmake = shutil.which(
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
        ).add_option("OPTION_SDK_TOOLS_BUILD_NUMBER", build_number).add_option(
            "Python_EXECUTABLE", sys.executable
        )

        if webengine:
            self.with_webengine()

        if gfxstream:
            self.with_gfxstream()

        if gfxstream_only:
            self.with_gfxstream_only()

        if thread_safety:
            self.with_thread_safety()

        self.with_ccache(self._find_ccache(aosp, ccache))

        if sanitizer:
            self.add_option("OPTION_ASAN", ",".join(sanitizer))

        if gfxstream_only:
            self.cmake_cmd.append(aosp / "device" / "generic" / "vulkan-cereal")
        else:
            self.cmake_cmd.append(aosp / "external" / "qemu")

        self._add_sdk_revision(aosp)
        self.env = get_default_environment(aosp)
        self.log_dir = Path(dist or destination) / "testlogs"
        self.destination = Path(destination)

        self.add_option("OPTION_TEST_LOGS", self.log_dir.absolute())

        self.with_features(aosp, features)
        self.with_options(options)

    def add_option(self, key: str, val: str):
        """
        Add an option to the CMake command.

        Args:
            key: The key of the option.
            val: The value of the option.

        Returns:
            Self.
        """
        if key and val:
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

        # Our build bots use sccache, so we will def. have it
        search_dir = os.path.join(
            aosp
            / "prebuilts"
            / "android-emulator-build"
            / "common"
            / "sccache"
            / f"{host}-x86_64"
        )
        logging.debug("Looking in %s for sccache", search_dir)
        return Path(shutil.which("sccache", path=search_dir))

    def with_ccache(self, ccache: str):
        return self.add_option("OPTION_CCACHE", ccache)

    def with_webengine(self):
        return self.add_option("QTWEBENGINE", True)

    def with_gfxstream(self):
        return self.add_option("GFXSTREAM", True)

    def with_gfxstream_only(self):
        self.add_option("ENABLE_VKCEREAL_TESTS", True)

    def with_thread_safety(self):
        return self.add_option("OPTION_CLANG_THREAD_SAFETY_CHECKS", True)

    def with_options(self, raw_options):
        if raw_options:
            self.cmake_cmd += [f"-D{x}" for x in raw_options]
        return self

    def with_features(self, aosp, raw_features):
        if raw_features:
            feature = FeatureParser(aosp / "external" / "qemu" / "CMakeLists.txt")
            self.cmake_cmd += [
                feature.feature_to_cmake_parameter(f) for f in raw_features
            ]

    def with_asan(self, sanitizer: [str]):
        return self.add_option("OPTION_ASAN", ",".join(sanitizer))

    def _bin_place_log_file(self, log_path: Path):
        """Copies a log file if it exists.

        Args:
            log_path (Path): Path to the logfile that needs to be copied.
        """
        if log_path.exists():
            self.log_dir.mkdir(exist_ok=True, parents=True)
            logging.info("Copy %s --> %s", log_path, self.log_dir / log_path.name)
            shutil.copyfile(log_path.absolute(), self.log_dir / log_path.name)

    def do_run(self):
        logging.info(
            "Configure target: %s, %s compilation",
            self.toolchain.target,
            "cross" if self.toolchain.is_crosscompile() else "native",
        )
        try:
            Command(self.cmake_cmd).with_environment(self.env).run()
        except CommandFailedException as cfe:
            self._bin_place_log_file(
                self.destination / "CMakeFiles" / "CMakeOutput.log"
            )
            self._bin_place_log_file(self.destination / "CMakeFiles" / "CMakeError.log")
            raise cfe
