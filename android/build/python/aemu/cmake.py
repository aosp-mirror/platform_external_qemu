#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
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
import multiprocessing
import os
import platform
import shutil
import sys
import logging

from aemu.definitions import (
    ENUMS,
    get_aosp_root,
    get_cmake,
    get_qemu_root,
    read_simple_properties,
    find_aosp_root,
    set_aosp_root,
)
from aemu.distribution import create_distribution
from aemu.process import run
from aemu.run_tests import run_tests


def ensure_requests():
    """Make sure the requests package is availabe."""
    try:
        import requests
    except:
        run([sys.executable, "-m", "ensurepip"])
        run([sys.executable, "-m", "pip", "install", "--user", "requests"])


def configure(args):
    """Configures the cmake project."""

    # Clear out the existing directory.
    if args.clean:
        if os.path.exists(args.out):
            logging.info("Clearing out %s", args.out)
            shutil.rmtree(args.out)
        if not os.path.exists(args.out):
            os.makedirs(args.out)

    # Configure..
    cmake_cmd = [get_cmake(), "-B%s" % args.out]

    # Setup the right toolchain/compiler configuration.
    cmake_cmd += [
        "-DCMAKE_TOOLCHAIN_FILE={}".format(
            os.path.join(
                get_qemu_root(),
                "android",
                "build",
                "cmake",
                ENUMS["Toolchain"][args.target],
            )
        )
    ]
    cmake_cmd += ENUMS["Crash"][args.crash]
    cmake_cmd += ENUMS["BuildConfig"][args.config]

    # Make darwin and msvc builds have QtWebEngine support for the default
    # build.
    if args.target == "darwin" or args.target == "windows":
        args.qtwebengine = True
    cmake_cmd += ["-DQTWEBENGINE=%s" % args.qtwebengine]

    if args.cmake_option:
        additional_cmake_args = ["-D%s" % x for x in args.cmake_option]
        logging.warning(
            "Dangerously adding the following args to cmake: %s", additional_cmake_args
        )
        cmake_cmd += additional_cmake_args

    if args.sdk_revision:
        sdk_revision = args.sdk_revision
    else:
        sdk_revision = read_simple_properties(
            os.path.join(get_aosp_root(), "external", "qemu", "source.properties")
        )["Pkg.Revision"]
    cmake_cmd += ["-DOPTION_SDK_TOOLS_REVISION=%s" % sdk_revision]

    if args.sdk_build_number:
        cmake_cmd += ["-DOPTION_SDK_TOOLS_BUILD_NUMBER=%s" % args.sdk_build_number]
    if args.sanitizer:
        cmake_cmd += ["-DOPTION_ASAN=%s" % (",".join(args.sanitizer))]

    if args.minbuild:
        cmake_cmd += ["-DOPTION_MINBUILD=%s" % args.minbuild]

    if args.gfxstream:
        cmake_cmd += ["-DGFXSTREAM=%s" % args.gfxstream]

    if args.crosvm:
        cmake_cmd += ["-DCROSVM=%s" % args.crosvm]

    if args.gfxstream_only:
        cmake_cmd += ["-DGFXSTREAM_ONLY=%s" % args.gfxstream_only]

    cmake_cmd += ENUMS["Generator"][args.generator]
    cmake_cmd += [get_qemu_root()]

    if args.crash != "none":
        # Make sure we have the requests library
        ensure_requests()

    run(cmake_cmd, args.out)


def get_build_cmd(args):
    """Gets the command that will build all the sources."""
    target = "install"
    cross_compile = platform.system().lower() != args.target

    # Stripping is meaning less in windows world, or when we are cross compiling.
    if (args.crash != "none" or args.strip != "none") and (
        platform.system().lower() != "windows" and not cross_compile
    ):
        target += "/strip"
    return [get_cmake(), "--build", args.out, "--target", target]


def main(args):
    version = sys.version_info
    logging.info(
        "Running under Python {0[0]}.{0[1]}.{0[2]}, Platform: {1}".format(
            version, platform.platform()
        )
    )

    configure(args)
    if args.build == "config":
        print("You can now build with: %s " % " ".join(get_build_cmd(args)))
        return

    # Build
    run(get_build_cmd(args))

    # Test.
    if args.tests:
        cross_compile = platform.system().lower() != args.target
        if not cross_compile:
            run_tests_opts = []
            if args.gfxstream or args.crosvm or args.gfxstream_only:
                run_tests_opts.append("--skip-emulator-check")

            run_tests(args.out, args.test_jobs, args.crash != "none", run_tests_opts)
        else:
            logging.info("Not running tests for cross compile.")

    # Create a distribution if needed.
    if args.dist:
        data = {
            "aosp": get_aosp_root(),
            "target": args.target,
            "sdk_build_number": args.sdk_build_number,
            "config": args.config,
        }

        create_distribution(args.dist, args.out, data)

    if platform.system() != "Windows" and args.config == "debug":
        overrides = open(
            os.path.join(get_qemu_root(), "android", "asan_overrides")
        ).read()
        print("Debug build enabled.")
        print(
            "ASAN may be in use; recommend disabling some ASAN checks as build is not"
        )
        print("universally ASANified. This can be done with")
        print()
        print(". android/envsetup.sh")
        print("")
        print("or export ASAN_OPTIONS=%s" % overrides)


def launch():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--sdk_revision",
        dest="sdk_revision",
        help="## DEPRECATED ##, it will automatically use the one defined in source.properties",
    )
    parser.add_argument("--sdk_build_number", help="The emulator sdk build number.")
    parser.add_argument(
        "--config",
        default="release",
        choices=(ENUMS["BuildConfig"].keys()),
        help="Whether we are building a release or debug configuration.",
    )
    parser.add_argument(
        "--crash",
        default="none",
        choices=(ENUMS["Crash"].keys()),
        help="Which crash server to use or none if you do not want crash uploads. Enabling this will result in symbol processing and uploading during install.",
    )
    parser.add_argument(
        "--out", default=os.path.abspath("objs"), help="Use specific output directory."
    )
    parser.add_argument("--dist", help="Create distribution in directory")
    parser.add_argument(
        "--qtwebengine",
        dest="qtwebengine",
        default=False,
        action="store_true",
        help="Build with QtWebEngine support",
    )
    parser.add_argument(
        "--no-qtwebengine",
        dest="qtwebengine",
        action="store_false",
        help="Build without QtWebEngine support",
    )
    parser.add_argument(
        "--no-tests", dest="tests", action="store_false", help="Do not run the tests"
    )
    parser.add_argument(
        "--tests",
        dest="tests",
        default=True,
        action="store_true",
        help="Run all the tests",
    )
    parser.add_argument(
        "--test_jobs",
        default=multiprocessing.cpu_count(),
        help="Specifies  the number of tests to run simultaneously",
        type=int,
    )
    parser.add_argument(
        "--sanitizer",
        action="append",
        help="List of sanitizers ([address, thread]) to enable in the built binaries.",
    )
    parser.add_argument(
        "--generator",
        default="ninja",
        choices=ENUMS["Generator"].keys(),
        help="CMake generator to use.",
    )
    parser.add_argument(
        "--target",
        default=platform.system().lower(),
        choices=ENUMS["Toolchain"].keys(),
        help="Which platform to target. This will attempt to cross compile if the target does not match the current platform ({})".format(
            (platform.system().lower())
        ),
    )
    parser.add_argument(
        "--build",
        default="check",
        choices=ENUMS["Make"].keys(),
        help="Target that should be build after configuration. The config target will only configure the build, no symbol processing or testing will take place.",
    )
    parser.add_argument(
        "--no-clean",
        dest="clean",
        default=True,
        action="store_false",
        help="Attempt an incremental build. Note that this can introduce cmake caching issues.",
    )
    parser.add_argument(
        "--clean",
        dest="clean",
        default=True,
        action="store_true",
        help="Clean the destination directory before building.",
    )
    parser.add_argument(
        "--cmake_option",
        action="append",
        help="Options that should be passed on directly to cmake. These will be passed on directly to the underlying cmake project. For example: "
        "--cmake_option QEMU_UPSTREAM=FALSE",
    )
    parser.add_argument(
        "--strip", dest="strip", action="store_true", help="Strip debug symbols."
    )
    parser.add_argument(
        "--no-strip",
        dest="strip",
        default=False,
        action="store_false",
        help="Do not strip debug symbols.",
    )

    parser.add_argument(
        "--minbuild",
        default=False,
        action="store_true",
        help="Minimize the build to only support x86_64/aarch64.",
    )

    parser.add_argument(
        "--gfxstream",
        action="store_true",
        dest="gfxstream",
        help="Build gfxstream libs/tests and crosvm",
    )
    parser.add_argument(
        "--crosvm", dest="crosvm", action="store_true", help="Build crosvm"
    )
    parser.add_argument(
        "--gfxstream_only",
        dest="gfxstream_only",
        action="store_true",
        help="Build only gfxstream libs/tests",
    )
    parser.add_argument(
        "--verbose",
        dest="verbose",
        default=False,
        action="store_true",
        help="Verbose logging",
    )
    parser.add_argument(
        "--aosp",
        dest="aosp",
        default=find_aosp_root(),
        help="AOSP directory ({})".format(find_aosp_root()),
    )

    args, leftover = parser.parse_known_args()

    # Configure logger.
    lvl = logging.DEBUG if args.verbose else logging.INFO
    handler = logging.StreamHandler()
    logging.root = logging.getLogger("root")
    logging.root.addHandler(handler)
    logging.root.setLevel(lvl)

    set_aosp_root(args.aosp)

    main(args)


if __name__ == "__main__":
    launch()
