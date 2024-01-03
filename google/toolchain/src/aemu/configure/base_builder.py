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
import platform
from collections import namedtuple
from pathlib import Path
from typing import Dict, Set, Tuple
from aemu.process.bazel import Bazel

from aemu.toolchains.toolchain_generator import ToolchainGenerator
from aemu.configure.package_config_pc import PackageConfigPc
from aemu.process.runner import run

LibInfo = namedtuple("LibInfo", ["bazel_target", "version", "shim"])


class QemuBuilder:
    """Configures the QEMU build for the specified platform.

    This class generates toolchain wrapper functions, a Meson toolchain configuration file,
    and package config files derived from the Bazel source build needed for QEMU. It can also
    invoke Meson setup to configure QEMU and optionally compile the source.

    Args:
        dest (str): The destination path for the configuration.
        generator (ToolchainGenerator): The toolchain generator to use.

    Attributes:
        TOOLCHAIN_DIR (str): The directory for the toolchain.
    """

    TOOLCHAIN_DIR = "toolchain"

    def __init__(self, aosp, dest, ccache, generator) -> None:
        """Initialize the QemuBuilder instance.

        Args:
            dest (str): The destination path for the configuration.
            generator_klazz (ToolchainGenerator): The toolchain generator instance.
        """
        self.aosp: Path = Path(aosp).absolute()
        self.dest: Path = Path(dest).absolute()
        self.dest.mkdir(parents=True, exist_ok=True)
        self.bazel: Bazel = Bazel(self.aosp, self.dest)

        # Initialize the toolchain generator with the specified destination and an empty suffix.
        # This generator will be used to manage toolchain-related configurations.
        self.toolchain_generator: ToolchainGenerator = generator
        if ccache:
            self.toolchain_generator.ccache = Path(ccache).absolute()
        self.toolchain_generator.bazel = self.bazel

    def host(self) -> str:
        return platform.system().lower()

    def get_library_config(
        self, bazel_target: str, shim: Dict[str, str]
    ) -> Tuple[Set[Path], Path]:
        """Get the public includes and archive path exported by the given Bazel target.

        Args:
            bazel_target (str): The Bazel target to query.
            shim (Dict[str, str]): Additional shims we wish to apply

        Returns:
            Tuple[Set[Path], Path]: A tuple containing a set of include paths and the archive path.
        """
        self.bazel.build_target(bazel_target)
        archive = shim.get("archive", self.bazel.get_archive(bazel_target))

        if "includes" in shim:
            # Apply shims, vs. what bazel reports.
            libdir = str(archive.parent)
            includes = list(
                set([Path(x.replace("${libdir}", libdir)) for x in shim["includes"]])
            )
        else:
            includes = self.bazel.get_includes(bazel_target)

        return includes, archive

    def generate_pkg_config(self, lib_info: LibInfo):
        """Generate a pkgconfig .pc file for the given Bazel target, registering it with a provided version.

        Apply specified shims if needed, including building the target, retrieving the archive,
        obtaining library includes, and creating the pkg-config discovery file.

        Args:
            bazel_target (str): The Bazel target to generate the pkg-config file for.
            version (str): The version to register in the pkg-config file.
            shim (Dict[str, str]): Shims to apply during the generation process.
        """
        # Build the specified Bazel target.
        self.bazel.build_target(lib_info.bazel_target)

        # Retrieve the information associated with the target.
        includes, archive = self.get_library_config(
            lib_info.bazel_target, lib_info.shim
        )

        # Name is @module//:<name>, i.e. the thing after :
        pkglib_name = lib_info.bazel_target[lib_info.bazel_target.rfind(":") + 1 :]

        cfg = PackageConfigPc(
            pkglib_name, lib_info.version, self.dest / "release", archive, includes, lib_info.shim
        )
        cfg.write(self.dest / self.TOOLCHAIN_DIR / ToolchainGenerator.PKGCFG_DIR)
        if not cfg.is_static():
            cfg.binplace(self.dest)

    def configure_meson(self):
        """Configure the QEMU build using Meson build system."""
        # Generate the toolchain wrappers.
        self.toolchain_generator.gen_toolchain()

        # # Write toolchain and host configurations.
        self.toolchain_generator.write_toolchain_config()

        # # Create pkgconfig .pc files for specified Bazel packages.
        for package in self.packages():
            self.generate_pkg_config(package)

        # Write the config-host.mak file.
        host_mak = f"# Automatically generated by qemu-build - do not modify \n\n"
        host_mak += "all:\n"
        with open(self.dest / "config-host.mak", "w", encoding="utf-8") as f:
            f.write("\n".join(self.config_mak()))

        # Invoke Meson setup with the proper configuration.
        run(
            [self.dest / QemuBuilder.TOOLCHAIN_DIR / "meson"]
            + ["setup", self.dest]
            + self.meson_config()
            + [self.dest / QemuBuilder.TOOLCHAIN_DIR / "aosp-cl.ini"],
            cwd=self.aosp / "external" / "qemu",
            toolchain_path=self.dest / QemuBuilder.TOOLCHAIN_DIR,
        )

    def packages(self):
        """Set of bazel targets for which we are going to generate pkgconfig discovery files.

        Subclasses should override these to provide platform specific libraries/shims that might be needed.
        """
        raise ValueError("Subclasses need to provide this.")

    def meson_config(self):
        """The set of meson configuration flags that need to be provided to meson setup to configure qemu"""
        raise ValueError("Subclasses need to provide this.")

    def config_mak(self):
        """The config.mak file needed by meson to configure qemu."""
        raise ValueError("Subclasses need to provide this.")
