#!/usr/bin/env python3
#
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
from collections import namedtuple
from pathlib import Path

from linux_generator import LinuxToLinuxGenerator
from windows_generator import WindowsToWindowsGenerator
from darwin_generator import DarwinToDarwinGenerator
from toolchain_generator import ToolchainGenerator
from log import configure_logging
from pkgconfig_generator import PkgConfigFromBazel

LibInfo = namedtuple("LibInfo", ["bazel_target", "version", "shim"])


class CustomFormatter(
    argparse.RawTextHelpFormatter, argparse.ArgumentDefaultsHelpFormatter
):
    pass


def gen_package_configs(dest: Path):
    bazel_configs = [
        #  TODO(jansene) auto derive dependencies.
        LibInfo("@zlib//:zlib", "1.2.10", {}),
        LibInfo("@glib//:glib-2.0", "2.77.2", {"includes" : "${libdir}", "Requires" : "pcre2"}),
        LibInfo("@pixman//:pixman-1", "0.42.3",  {"Requires" : "pixman_simd"}),
        LibInfo("@pixman//:pixman_simd", "0.42.3", {}),
        LibInfo("@pcre2//:pcre2", "10.42", {}),
        # etc.. etc..
        # LibInfo("/external/curl//:curl", "0.21.8"),
        # LibInfo("/external/libseccomp//:libseccomp", "0.21.8"),
    ]
    cfg = PkgConfigFromBazel(dest)
    for bazel in bazel_configs:
        logging.debug("Generating %s", bazel)
        cfg.gen_pkg_config(bazel.bazel_target, bazel.version, bazel.shim)


def gen_toolchain(target: str, dest: Path, prefix: str):
    generator_map = {
        "emulator-windows_x64": WindowsToWindowsGenerator,
        "windows": WindowsToWindowsGenerator,
        "emulator-linux_x64": LinuxToLinuxGenerator,
        "linux": LinuxToLinuxGenerator,
        "emulator-mac_aarch64": DarwinToDarwinGenerator,
        "darwin": DarwinToDarwinGenerator,
    }
    if target not in generator_map:
        raise ValueError(f"No toolchain support for target: {target}")

    gen = generator_map[target](dest, prefix)
    gen.gen_toolchain()


def main():
    parser = argparse.ArgumentParser(
        formatter_class=CustomFormatter,
        description=f"""
        Toolchain wrapper script generator.

        This script is able to create a set of wrappers that can be used to compile QEMU.
        The wrapper will create a set of toolchain files (cc, c++, nm, etc...) that will
        properly invoke the clang (or clang-cl) compiler that ships with AOSP

        The clang version will be obtained from {ToolchainGenerator.TOOLCHAIN_JSON}.

        It will also create a set of fake "pkgconfig" files that will resolve to the
        dependencies that will be built using bazel.

        Note that this does not support cross compilation.
        """,
    )
    parser.add_argument(
        "--target",
        type=str,
        default=platform.system().lower(),
        help="Toolchain target",
    )
    parser.add_argument(
        "out",
        type=str,
        help="Destination directory where the toolchain should be written",
    )

    parser.add_argument(
        "--prefix",
        dest="prefix",
        type=str,
        default="",
        help="Compiler prefix to use, e.g. my-pre-c++",
    )
    parser.add_argument(
        "-f",
        "--force",
        dest="force",
        default=False,
        action="store_true",
        help="Ignore existing directory and overwrite existing files",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        default=False,
        action="store_true",
        help="Verbose logging",
    )

    args = parser.parse_args()
    lvl = logging.DEBUG if args.verbose else logging.INFO
    configure_logging(lvl)

    out = Path(args.out).absolute()

    if out.exists() and not args.force:
        logging.fatal(
            "The directory %s already exists, please delete it first or use the -f flag.",
            out,
        )
        return

    out.mkdir(exist_ok=True, parents=True)

    gen_toolchain(args.target, out, args.prefix)
    gen_package_configs(out)


if __name__ == "__main__":
    main()
