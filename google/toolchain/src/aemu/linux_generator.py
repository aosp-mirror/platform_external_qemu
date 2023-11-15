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
import shutil
import subprocess
from pathlib import Path

from toolchain_generator import AOSP_ROOT, ToolchainGenerator


class LinuxToLinuxGenerator(ToolchainGenerator):
    TOOLCHAIN_DIR = (
        AOSP_ROOT
        / "prebuilts"
        / "gcc"
        / "linux-x86"
        / "host"
        / "x86_64-linux-glibc2.17-4.8"
    )
    GCC_DIR = TOOLCHAIN_DIR / "lib" / "gcc" / "x86_64-linux" / "4.8.3"
    SYSROOT = TOOLCHAIN_DIR / "sysroot"
    COMPAT_ARCHIVE = "//external/qemu/google/compat:compat"

    def __init__(self, dest, prefix) -> None:
        super().__init__(dest, prefix)
        version_path = (
            self.clang() / "lib" / "clang" / self.compiler_version
        ).absolute()
        self.compat_path = (
            AOSP_ROOT / "external" / "qemu" / "google" / "compat" / "linux"
        ).absolute()


        self.run([self.bazel.exe, "build", self.COMPAT_ARCHIVE])
        self.compat_path_a = self.get_archive(self.COMPAT_ARCHIVE).with_suffix("")

        self.link_flags = (
            f"--gcc-toolchain={self.TOOLCHAIN_DIR}"
            f" -B{self.TOOLCHAIN_DIR}/lib/gcc/x86_64-linux/4.8.3/"
            f" -L{self.TOOLCHAIN_DIR}/lib/gcc/x86_64-linux/4.8.3/"
            f" -L{self.TOOLCHAIN_DIR}/x86_64-linux/lib64/"
            f" -L{self.dest}/lib"
            " -fuse-ld=lld"
            f" -L{version_path}/lib/linux/"
            f" -L{self.clang()}/lib/"
            f" --sysroot={self.SYSROOT}"
            f" -Wl,-rpath,'$ORIGIN/lib64:$ORIGIN:{self.clang() / 'lib'}'"
            f" -L{self.compat_path_a.parent}"
        )
        self.isystem_flags = (
            f"-isystem {self.SYSROOT}/usr/include "
            f"-isystem {self.SYSROOT}/usr/include/x86_64-linux-gnu "
        )

    def get_archive(self, bazel_target: str) -> Path:
        archives = subprocess.check_output(
            [
                self.bazel.exe,
                "cquery",
                "--output=starlark",
                "--starlark:expr='\\n'.join([f.path for f in target.files.to_list()])",
                bazel_target,
            ],
            text=True,
        ).splitlines()

        archives = [
            x.strip().replace("bazel-out", self.bazel.bazel_out)
            for x in archives
            if x.endswith(".a")
        ]
        return Path(archives[0])

    def gen_toolchain(self):
        super().gen_toolchain()
        (self.dest / "lib").mkdir(exist_ok=True)
        self.gen_script(self.dest / "pkg-config", self.pkg_config)
        source_directory = self.SYSROOT / "usr" / "lib" / "pkgconfig"
        destination_directory = self.dest / "pkgconfig"
        destination_directory.mkdir(parents=True, exist_ok=True)
        for source_file in source_directory.iterdir():
            destination_file = destination_directory / source_file.name
            shutil.copy2(source_file, destination_file)

    def pkg_config(self):
        real_pkg = Path(shutil.which("pkg-config")).absolute()
        script = f"PKG_CONFIG_PATH={self.dest / 'pkgconfig'} {real_pkg} --env-only"
        return script, ""


    def cc(self):
        script = (
            f"{self.clang()}/bin/clang "
            "-m64 -march=native -mtune=native -mcx16 "
            f"{self.link_flags} "
            f"-isystem {self.compat_path} "
        )
        extra = (
            "-Wno-unused-command-line-argument -lcompat -lc++ -ldl "
            f"{self.isystem_flags} "
        )
        return script, extra

    def cxx(self):
        script = (
            f"{self.clang()}/bin/clang++ "
            "-m64 -march=native -mtune=native -mcx16 "
            "-stdlib=libc++ "
            f"{self.link_flags} "
            f"-isystem {self.compat_path} "
        )

        extra = (
            "-Wno-unused-command-line-argument -lcompat -lc++ -ldl "
            f"{self.isystem_flags} "
        )
        return script, extra
