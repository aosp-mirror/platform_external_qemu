#!/usr/bin/env python3
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
import argparse
import logging
import platform
import shutil
import sys
import zipfile
from pathlib import Path

from aemu.configure.base_builder import QemuBuilder
from aemu.configure.darwin_builder import DarwinBuilder
from aemu.configure.linux_builder import LinuxBuilder
from aemu.configure.trusty_builder import TrustyBuilder
from aemu.configure.windows_builder import WindowsBuilder
from aemu.log import configure_logging
from aemu.process.runner import run
from aemu.toolchains.darwin_generator import (
    DarwinToDarwinGenerator,
    DarwinToDarwinX64Generator,
)
from aemu.toolchains.linux_arm_generator import LinuxToLinuxAarch64Generator
from aemu.toolchains.linux_generator import LinuxToLinuxGenerator
from aemu.toolchains.toolchain_generator import ToolchainGenerator
from aemu.toolchains.windows_generator import WindowsToWindowsGenerator


class CustomFormatter(
    argparse.RawTextHelpFormatter, argparse.ArgumentDefaultsHelpFormatter
):
    pass


TARGET_ALIAS = {
    "emulator-windows_x64": "windows-x64",
    "windows": "windows-x64",
    "windows-msvc-x86_64": "windows-x64",
    "linux": "linux-x64",
    "trusty": "linux-x64",
    "emulator-linux_x64": "linux-x64",
    "linux-x86_64": "linux-x64",
    "linux-aarch64": "linux-aarch64",
    "darwin": "mac-aarch64",
    "darwin-x86_64": "mac-x64",
    "darwin-aarch64": "mac-aarch64",
    "emulator-mac_aarch64": "mac-aarch64",
}


def get_toolchain_generator(
    target: str, dest: Path, prefix: str, aosp: Path, ccache: Path
) -> ToolchainGenerator:
    # TODO: __host__To__target__ Toolchain generator.
    # Note that if you wish to add cross compilation support
    # You will have to add this support to the bazel toolchains
    # as well, as we depend on bazel for pkg-config lib + include
    # generation.
    generator_map = {
        "windows-x64": WindowsToWindowsGenerator,
        "linux-x64": LinuxToLinuxGenerator,
        "linux-aarch64": LinuxToLinuxAarch64Generator,
        "mac-aarch64": DarwinToDarwinGenerator,
        "mac-x64": DarwinToDarwinX64Generator,
    }

    if target not in TARGET_ALIAS:
        raise ValueError(f"No toolchain support for target: {target}")

    toolchain_klazz = generator_map[TARGET_ALIAS[target]]
    # Initialize the toolchain generator with the specified destination and an empty suffix.
    # This generator will be used to manage toolchain-related configurations.
    return toolchain_klazz(Path(aosp), Path(dest), prefix)


def gen_toolchain(
    target: str, dest: Path, prefix: str, aosp: Path, ccache: Path
) -> QemuBuilder:
    builder_map = {
        "windows-x64": WindowsBuilder,
        "linux-x64": LinuxBuilder,
        "linux-aarch64": LinuxBuilder,
        "mac-aarch64": DarwinBuilder,
        "trusty": TrustyBuilder,
        "mac-x64": DarwinBuilder,
    }

    # Get the class that is capable of configuring the toolchain
    # from the current host targeting our target.
    toolchain = get_toolchain_generator(
        target, Path(dest) / QemuBuilder.TOOLCHAIN_DIR, "", aosp, ccache
    )

    if target not in builder_map:
        logging.info("Mapping %s -> %s", target, TARGET_ALIAS[target])
        target = TARGET_ALIAS[target]

    return builder_map[target](Path(aosp), Path(dest), ccache, toolchain)


def mkdirs(out: Path, force: bool):
    if out.exists():
        if force:
            shutil.rmtree(out)
        else:
            logging.fatal(
                "The directory %s already exists, please delete it first or use the -f flag.",
                out,
            )
            raise FileExistsError(f"The directory {out} already exists")


def setup_command(args):
    mkdirs(Path(args.out).absolute(), args.force)
    builder = gen_toolchain(
        args.target, Path(args.out), args.prefix, args.aosp, args.ccache
    )
    builder.configure_meson()


def toolchain_command(args):
    mkdirs(Path(args.out).absolute(), args.force)
    toolchain = get_toolchain_generator(
        args.target, Path(args.out), args.prefix, args.aosp, args.ccache
    )
    toolchain.gen_toolchain()


def compile_command(args):
    """Compile the QEMU source by invoking Meson.

    This method compiles the QEMU source by invoking Meson's compile command.
    """
    run(
        [
            Path(args.out) / QemuBuilder.TOOLCHAIN_DIR / "meson",
            "compile",
            "-C",
            args.out,
        ],
        cwd=args.out,
        toolchain_path=Path(args.out) / QemuBuilder.TOOLCHAIN_DIR,
    )


def test_command(args):
    """Run the QEMU tests by invoking Meson."""
    run(
        [
            Path(args.out) / QemuBuilder.TOOLCHAIN_DIR / "meson",
            "test",
            "--print-errorlogs",
            "-C",
            args.out,
        ],
        cwd=args.out,
        toolchain_path=Path(args.out) / QemuBuilder.TOOLCHAIN_DIR,
    )


def release_command(args):
    """Run the QEMU tests by invoking Meson."""
    run(
        [
            Path(args.out) / QemuBuilder.TOOLCHAIN_DIR / "meson",
            "install",
            "-C",
            args.out,
        ],
        cwd=args.out,
        toolchain_path=Path(args.out) / QemuBuilder.TOOLCHAIN_DIR,
    )

    logging.info("Creating %s", args.release)
    with zipfile.ZipFile(
        args.release, "w", zipfile.ZIP_DEFLATED, allowZip64=True
    ) as zipf:
        search_dir = Path(args.out) / "release"
        for fname in search_dir.glob("**/*"):
            arcname = fname.relative_to(search_dir)
            logging.info("Adding %s as %s", fname, arcname)
            zipf.write(fname, arcname)


def find_aosp_root(start_directory=Path(__file__).resolve()):
    current_directory = Path(start_directory).resolve()

    while True:
        repo_directory = current_directory / ".repo"
        if repo_directory.is_dir():
            return str(current_directory)

        # Move up one directory
        parent_directory = current_directory.parent

        # Check if we've reached the root directory
        if current_directory == parent_directory:
            return str(current_directory)

        current_directory = parent_directory


def main():
    parser = argparse.ArgumentParser(
        formatter_class=CustomFormatter,
        description=f"""
        Android Meson Configurator

        This script is able to create a set of wrappers that can be used to compile QEMU.
        The wrapper will create a set of toolchain files (cc, c++, nm, etc...) that will
        properly invoke the clang (or clang-cl) compiler that ships with AOSP, installing
        proper shims where needed.

        It is expected that this script will be run with access to the AOSP_ROOT from a manifest
        that is based of `emu-dev` (https://android.googlesource.com/platform/manifest/+/refs/heads/emu-dev)

        The clang version will be obtained from $AOSP_ROOT/build/bazel/rules/toolchains.json

        It will also create a set of fake "pkgconfig" files that will resolve to the
        dependencies that will be built using bazel.

        Note that this does not support cross compilation.
        """,
    )

    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        default=False,
        action="store_true",
        help="Verbose logging",
    )

    subparsers = parser.add_subparsers(title="Commands", dest="command", metavar="")

    # Subparser for 'toolchain' command
    toolchain_parser = subparsers.add_parser(
        "toolchain", help="Create toolchain wrappers for compilation"
    )
    toolchain_parser.set_defaults(func=toolchain_command)
    toolchain_parser.add_argument(
        "out", type=str, help="Directory for toolchain wrappers"
    )
    toolchain_parser.add_argument(
        "--ccache",
        dest="ccache",
        default=shutil.which("ccache") or shutil.which("sccache"),
        help="Use the given compiler cache (ccache/sccache)",
    )
    toolchain_parser.add_argument(
        "-f",
        "--force",
        dest="force",
        default=False,
        action="store_true",
        help="Ignore existing directory and overwrite existing files",
    )
    toolchain_parser.add_argument(
        "--aosp",
        type=str,
        default=find_aosp_root(),
        help="AOSP root to use",
    )
    toolchain_parser.add_argument(
        "--prefix",
        dest="prefix",
        type=str,
        default="",
        help="Compiler prefix to use, e.g. my-pre-c++",
    )
    toolchain_parser.add_argument(
        "--target",
        type=str,
        default=platform.system().lower(),
        help="Toolchain target, the host you wish to run the executables on",
    )

    # Subparser for 'setup' command
    setup_parser = subparsers.add_parser(
        "setup", help="Create wrappers for QEMU compilation"
    )
    setup_parser.add_argument("out", type=str, help="Directory for toolchain wrappers")
    setup_parser.set_defaults(func=setup_command)
    setup_parser.add_argument(
        "--ccache",
        dest="ccache",
        default=shutil.which("ccache") or shutil.which("sccache"),
        help="Use the given compiler cache (ccache/sccache)",
    )
    setup_parser.add_argument(
        "-f",
        "--force",
        dest="force",
        default=False,
        action="store_true",
        help="Ignore existing directory and overwrite existing files",
    )
    setup_parser.add_argument(
        "--aosp",
        type=str,
        default=find_aosp_root(),
        help="AOSP root to use",
    )
    setup_parser.add_argument(
        "--prefix",
        dest="prefix",
        type=str,
        default="",
        help="Compiler prefix to use, e.g. my-pre-c++",
    )
    setup_parser.add_argument(
        "--target",
        type=str,
        default=platform.system().lower(),
        help="Toolchain target, the host you wish to run the executables on",
    )

    # Subparser for 'compile' command
    compile_parser = subparsers.add_parser("compile", help="Compile the QEMU source")
    compile_parser.set_defaults(func=compile_command)
    compile_parser.add_argument("out", type=str, help="Configured compile directory")

    # Subparser for 'test' command
    test_parser = subparsers.add_parser("test", help="Run QEMU tests")
    test_parser.set_defaults(func=test_command)
    test_parser.add_argument("out", type=str, help="Configured compile directory")

    # Subparser for 'release' command
    release_parser = subparsers.add_parser("release", help="Generate release zip")
    release_parser.set_defaults(func=release_command)
    release_parser.add_argument("out", type=str, help="Configured compile directory")
    release_parser.add_argument("release", type=str, help="Zipfile with the release")

    args = parser.parse_args()

    lvl = logging.DEBUG if args.verbose else logging.INFO
    configure_logging(lvl)

    # Make sure we use absolute paths, so we do not get
    # confused.
    if hasattr(args, "out") and args.out:
        args.out = Path(args.out).absolute()

    # Call the function associated with the selected subcommand
    if hasattr(args, "func"):
        try:
            args.func(args)
        except Exception as e:
            if args.verbose:
                raise e
            logging.fatal("Build failure: %s", e)
            sys.exit(1)

    else:
        parser.print_help()


if __name__ == "__main__":
    main()