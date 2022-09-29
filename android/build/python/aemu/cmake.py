#!/usr/bin/env python3
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
import logging
import multiprocessing
import os
import platform
import shutil
import sys
import time
from distutils.spawn import find_executable

from aemu.definitions import (ENUMS, find_aosp_root, get_aosp_root, get_cmake,
                              get_qemu_root, infer_target, is_crosscompile,
                              read_simple_properties, set_aosp_root)
from aemu.distribution import create_distribution
from aemu.process import run
from aemu.run_tests import run_tests


class LogBelowLevel(logging.Filter):
    def __init__(self, exclusive_maximum, name=""):
        super(LogBelowLevel, self).__init__(name)
        self.max_level = exclusive_maximum

    def filter(self, record):
        return True if record.levelno < self.max_level else False


def configure(args, target):
    """Configures the cmake project."""

    # Clear out the existing directory.
    toolchain = ENUMS["Toolchain"][target]
    if args.clean:
        if os.path.exists(args.out):
            logging.info("Clearing out %s", args.out)
            if platform.system() == "Windows":
                run(["rmdir", "/S", "/Q", args.out])
            else:
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
                toolchain,
            )
        )
    ]
    cmake_cmd += ENUMS["Crash"][args.crash]
    cmake_cmd += ENUMS["BuildConfig"][args.config]

    # Make darwin (x86_64 and aarch64) and msvc builds have QtWebEngine support for the default
    # build.
    build_qtwebengine = False
    if args.qtwebengine and args.no_qtwebengine:
        logging.error(
            "--qtwebengine and --no-qtwebengine cannot be set at the same time!"
        )
        sys.exit(1)
    if (
        args.qtwebengine
        or args.target == "darwin"
        or args.target == "darwin_aarch64"
        or args.target == "windows"
    ):
        build_qtwebengine = True
    if args.no_qtwebengine:
        build_qtwebengine = False
    cmake_cmd += ["-DQTWEBENGINE=%s" % build_qtwebengine]

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

    if args.gfxstream_only:
        cmake_cmd += ["-DGFXSTREAM_ONLY=%s" % args.gfxstream_only]

    ccache = find_ccache(args.ccache, platform.system().lower())
    if ccache:
        cmake_cmd += ["-DOPTION_CCACHE=%s" % ccache]

    cmake_cmd += ENUMS["Generator"][args.generator]
    cmake_cmd += [get_qemu_root()]

    run(cmake_cmd, args.out)


def get_build_cmd(args):
    """Gets the command that will build all the sources."""
    target = "install"

    # Stripping is meaning less in windows world
    if (args.crash != "none" or args.strip != "none") and (
        platform.system().lower() != "windows"
    ):
        target += "/strip"
    return [get_cmake(), "--build", args.out, "--target", target]


def find_ccache(cache, host):
    """Locates the cache executable as a posix path"""
    if cache == "none" or cache == None:
        return None
    if cache != "auto":
        return cache.replace("\\", "/")

    if host != "windows":
        # prefer ccache for local builds, it is slightly faster
        # but does not work on windows.
        ccache = find_executable("ccache")
        if ccache:
            return ccache

    # Our build bots use sccache, so we will def. have it
    search_dir = os.path.join(
        get_aosp_root(),
        "prebuilts",
        "android-emulator-build",
        "common",
        "sccache",
        "{}-x86_64".format(host),
    )
    return find_executable("sccache", search_dir).replace("\\", "/")


def main(args):
    start_time = time.time()
    version = sys.version_info
    target = infer_target(args.target)
    iscross = is_crosscompile(target)
    logging.info(
        "Running under Python {0[0]}.{0[1]}.{0[2]}, Platform: {1}, target: {2}, {3} compilation".format(
            version, platform.platform(), target, "cross" if iscross else "native"
        )
    )

    configure(args, target)
    if args.build == "config":
        print("You can now build with: %s " % " ".join(get_build_cmd(args)))
        return

    # Build
    run(get_build_cmd(args), None, {"NINJA_STATUS": "[ninja] "})
    logging.info("Completed build in %d seconds", time.time() - start_time)

    # Test.
    if args.tests:
        if not iscross:
            run_tests_opts = []
            if args.gfxstream or args.gfxstream_only:
                run_tests_opts.append("--skip-emulator-check")
            if args.gfxstream or args.gfxstream_only:
                run_tests_opts.append("--gfxstream")
            run_tests(args.out, args.dist, args.test_jobs, run_tests_opts)
            logging.info("Completed testing.")
        else:
            logging.info("Not running tests for cross compile or darwin aarch64.")

    # Create a distribution if needed.
    if args.dist:
        data = {
            "aosp": get_aosp_root(),
            "target": args.target,
            "sdk_build_number": args.sdk_build_number,
            "config": args.config,
        }
        logging.info("Creating distribution.")
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
        help="Enforce building with QtWebEngine support (cannot be used together with --no-qtwebengine)",
    )
    parser.add_argument(
        "--no-qtwebengine",
        dest="no_qtwebengine",
        default=False,
        action="store_true",
        help="Enforce building without QtWebEngine support (cannot be used together with --qtwebengine)",
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
        help="Which platform to target (windows|linux|darwin)([-_](aarch64|x86_64))?. The system will infer the architecture if possible. Supported targets: [{}],  Default: ({}). ".format(
            ", ".join(ENUMS["Toolchain"].keys()),
            infer_target(platform.system().lower()),
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
        "--cmake_option WEBRTC=TRUE",
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
        help="Build gfxstream libs/tests",
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

    parser.add_argument(
        "--ccache",
        help="Use a compiler cache, set to auto to infer on, or none if omitted.",
    )

    args, leftover = parser.parse_known_args()

    # Configure logging, splitting info to stdout, and error to stderr
    lvl = logging.DEBUG if args.verbose else logging.INFO
    logging_handler_out = logging.StreamHandler(sys.stdout)
    logging_handler_out.setLevel(logging.DEBUG)
    logging_handler_out.addFilter(LogBelowLevel(logging.WARNING))

    logging_handler_err = logging.StreamHandler(sys.stderr)
    logging_handler_err.setLevel(logging.WARNING)

    logging.root = logging.getLogger("root")
    logging.root.setLevel(lvl)
    logging.root.addHandler(logging_handler_out)
    logging.root.addHandler(logging_handler_err)

    set_aosp_root(args.aosp)

    try:
        main(args)
    except (Exception, KeyboardInterrupt) as exc:
        sys.exit(exc)


if __name__ == "__main__":
    launch()
