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

from aemu.toolchains.toolchain_generator import ToolchainGenerator


class LinuxToLinuxGenerator(ToolchainGenerator):
    COMPAT_ARCHIVE = "//external/qemu/google/compat/linux:compat"

    def initialize(self):
        if hasattr(self, "initialized"):
            return

        version_path = (self.clang() / "lib" / "clang" / self.cc_version()).absolute()

        compat_isystem_path = (
            self.aosp / "external" / "qemu" / "google" / "compat" / "linux" / "include"
        ).absolute()

        self.linux_sys_root = (
            self.aosp
            / "prebuilts"
            / "gcc"
            / "linux-x86"
            / "host"
            / "x86_64-linux-glibc2.17-4.8"
        )
        # GCC_DIR = TOOLCHAIN_DIR / "lib" / "gcc" / "x86_64-linux" / "4.8.3"
        self.system_root = self.linux_sys_root / "sysroot"
        self.bazel.build_target(self.COMPAT_ARCHIVE)
        compat_lib_dir = self.bazel.get_archive(self.COMPAT_ARCHIVE).parent
        linux_lib_path = version_path / "lib" / "linux"
        lib_path = self.clang() / "lib"
        include_path = version_path / "include"

        self.cflags = (
            f"--gcc-toolchain={self.linux_sys_root} "
            f"-B{self.linux_sys_root}/lib/gcc/x86_64-linux/4.8.3/ "
            f"-L{self.linux_sys_root}/lib/gcc/x86_64-linux/4.8.3/ "
            f"-L{self.linux_sys_root}/x86_64-linux/lib64/ "
            "-fuse-ld=lld -Wl,--allow-shlib-undefined "
            f"-L{linux_lib_path} "
            f"-L{include_path} "
            f"-L{lib_path} "
            f"--sysroot={self.system_root} "
            f"-Wl,-rpath,'$ORIGIN/lib64:$ORIGIN:{self.clang() / 'lib'}' "
            f"-L{compat_lib_dir} "
            f"-isystem {compat_isystem_path} "
        )
        self.initialized = True

    def strip(self):
        objcopy = self.clang() / "bin" / "llvm-objcopy"
        script = "target=$(basename $1)\n"
        script += f'{objcopy} --only-keep-debug  $1 "build/debug_info/$target.debug" \n'
        script += f"{objcopy} --strip-unneeded  $1\n"
        script += f'{objcopy} --add-gnu-debuglink="build/debug_info/$target.debug" $1\n'
        script += "# EXPLICITLY DISABLED ARBITRARY ARGUMENTS: "
        return script, ""

    def cc(self):
        self.initialize()
        cache = f"{self.ccache}" if self.ccache else ""
        script = (
            f"{cache} {self.clang()}/bin/clang "
            "-m64 -march=native -mtune=native -mcx16 "
            f"{self.cflags} "
        )
        extra = "-Wno-unused-command-line-argument -lcompat -lc++ -ldl "
        return script, extra

    def cxx(self):
        self.initialize()
        cache = f"{self.ccache}" if self.ccache else ""
        script = (
            f"{cache} {self.clang()}/bin/clang++ "
            "-m64 -march=native -mtune=native -mcx16 "
            "-stdlib=libc++ "
            f"{self.cflags} "
        )

        extra = "-Wno-unused-command-line-argument -lcompat -lc++ -ldl "
        return script, extra

    def link_dirs(self):
        """Setup links to libc++.so etc.."""
        super().link_dirs()
        (self.dest / "sysroot").symlink_to(self.system_root)
