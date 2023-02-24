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
import platform
import sys
from pathlib import Path
from typing import List

from aemu.platform.log_configuration import configure_logging
from aemu.platform.toolchains import Toolchain
from aemu.tasks.build_task import BuildTask
from aemu.tasks.clean import CleanTask
from aemu.tasks.compile import CompileTask
from aemu.tasks.configure import ConfigureTask
from aemu.tasks.distribution import DistributionTask
from aemu.tasks.gen_entries import GenEntriesTestTask
from aemu.tasks.integration_tests import IntegrationTestTask
from aemu.tasks.unit_tests import AccelerationCheckTask, CTestTask, CoverageReportTask
from aemu.tasks.emugen_test import EmugenTestTask
from aemu.tasks.kill_netsimd import KillNetsimdTask
from aemu.tasks.package_samples import PackageSamplesTask
from aemu.tasks.fix_cargo import FixCargoTask
from aemu.util.simple_feature_parser import FeatureParser


def get_tasks(args) -> List[BuildTask]:
    """A list of tasks that should be executed.

    Args:
        args: Parsed commandline arguments

    Returns:
        list[BuildTask]: List of tasks that need to be executed.
    """
    run_tests = not Toolchain(args.aosp, args.target).is_crosscompile()
    tasks = [
        KillNetsimdTask(),
        FixCargoTask(args.aosp).enable(False),
        # A task can be disabled, or explicitly enabled by calling
        # .enable(False) <- Disable the task
        CleanTask(args.out),
        ConfigureTask(
            aosp=args.aosp,
            target=args.target,
            destination=args.out,
            crash=args.crash,
            configuration=args.config,
            build_number=args.sdk_build_number,
            sanitizer=args.sanitizer,
            webengine=(
                args.qtwebengine
                or args.target == "darwin"
                or args.target == "darwin_aarch64"
                or args.target == "windows"
            ),
            gfxstream=args.gfxstream,
            gfxstream_only=args.gfxstream_only,
            options=args.cmake_option,
            ccache=args.ccache,
            thread_safety=args.thread_safety,
            dist=args.dist,
            features=args.feature,
        ),
        CompileTask(args.aosp, args.out),
    ]
    if not args.gfxstream_only:
        tasks += [
            CTestTask(
                aosp=args.aosp,
                destination=args.out,
                concurrency=args.test_jobs,
                with_gfx_stream=args.gfxstream or args.gfxstream_only,
                distribution_directory=args.dist,
            ).enable(run_tests),
            KillNetsimdTask().enable(run_tests),
            AccelerationCheckTask(args.out).enable(run_tests),
            EmugenTestTask(args.aosp, args.out).enable(run_tests).enable(False),
            GenEntriesTestTask(args.aosp, args.out),
            # Enable the integration tests by default once they are stable enough
            IntegrationTestTask(args.aosp, args.out, args.dist).enable(False),
            CoverageReportTask(aosp=args.aosp, destination=args.out).enable(run_tests),
            PackageSamplesTask(
                args.aosp, args.out, args.dist, args.target, args.sdk_build_number
            ),
            DistributionTask(
                aosp=args.aosp,
                build_directory=args.out,
                distribution_directory=args.dist,
                target=args.target,
                sdk_build_number=args.sdk_build_number,
                configuration=args.config,
            ).enable(args.dist is not None),
        ]
    return tasks


def main(args):
    logging.info("cmake.py %s", " ".join(sys.argv[1:]))

    if args.feature_list:
        FeatureParser(
            Path(args.aosp) / "external" / "qemu" / "CMakeLists.txt"
        ).list_help()
        return

    tasks = get_tasks(args)
    if args.task_list:
        for task in tasks:
            print(f"{task.name} :  {task.__class__.__doc__}")
        return

    if args.task:
        # Get explicit task you wish to run
        tasks = [x.enable(True) for x in tasks if x.name.lower() == args.task.lower()]

    enabled = [x.lower() for x in args.task_enable] if args.task_enable else []
    disabled = [x.lower() for x in args.task_disable] if args.task_disable else []
    for task in tasks:
        if task.name.lower() in enabled:
            task.enable(True)
        if task.name.lower() in disabled:
            task.enable(False)

        # A failing task should raise an exception.
        task.run()


def launch():
    parser = argparse.ArgumentParser(
        description="Configures the emulator build and runs the tests."
        "The build works by executing a series of tasks. "
        "You can enable/disable tasks or execute a specific task. "
        "Task names are case insensitive."
        "Note that tasks are very simple without any dependency resolution.",
    )
    parser.add_argument(
        "--sdk_build_number",
        default="standalone-0",
        help="The emulator sdk build number.",
    )
    parser.add_argument(
        "--config",
        default="release",
        choices=["debug", "release"],
        help="Whether we are building a release or debug configuration.",
    )
    parser.add_argument(
        "--crash",
        default="none",
        choices=["none", "prod", "staging"],
        help="Which crash server to use or none if you do not want crash uploads. Enabling this will result in symbol processing and uploading during install.",
    )
    parser.add_argument(
        "--out",
        default=Path("objs").absolute(),
        help="Use specific output directory, defaults to: objs",
    )
    parser.add_argument(
        "--dist",
        help="The distribution directory, or empty if no distribution should be created.",
    )
    parser.add_argument(
        "--qtwebengine",
        dest="qtwebengine",
        default=False,
        action="store_true",
        help="Enable location ui, with QtWebegine.",
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
        "--enable-thread-safety-checks",
        dest="thread_safety",
        default=False,
        action="store_true",
        help="Enable clang thread-safety compile-time checks (-Wthread-safety)",
    )
    parser.add_argument(
        "--target",
        default=platform.system().lower(),
        help="Which platform to target (windows|linux|darwin)([-_](aarch64|x86_64))?. The system will infer the architecture if possible",
    )
    parser.add_argument(
        "--cmake_option",
        action="append",
        help="Options that should be passed on directly to cmake. These will be passed on directly to the underlying cmake project. For example: "
        "--cmake_option WEBRTC=TRUE",
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
    aosp = Path(__file__).parents[6]
    parser.add_argument(
        "--aosp", dest="aosp", default=aosp, help=f"AOSP directory: {aosp}"
    )
    parser.add_argument(
        "--ccache",
        help="Use a compiler cache, set to auto to infer on, or none if omitted.",
    )
    parser.add_argument(
        "--task",
        help="Run the specified task. (Note no dependency analysis!)",
    )
    parser.add_argument(
        "--task-enable",
        action="append",
        help="Explicitly enable this task, for example --task-enable integrationtest",
    )
    parser.add_argument(
        "--task-disable",
        action="append",
        help="Explicitly disable this task, for example --task-disable clean",
    )
    parser.add_argument(
        "--task-list",
        action="store_true",
        help="Lists all the tasks.",
    )
    parser.add_argument(
        "--feature",
        action="append",
        help="Explicitly enable/disable a feature, for example --feature no-rust",
    )
    parser.add_argument(
        "--feature-list",
        action="store_true",
        help="Lists all the available features that can be enabled or disabled.",
    )

    args, leftover = parser.parse_known_args()

    lvl = logging.DEBUG if args.verbose else logging.INFO
    configure_logging(lvl)

    if leftover:
        logging.warning("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-")
        logging.warning(
            "Leftover arguments: %s, are ignored and will soon be removed.", leftover
        )
        logging.warning("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-")

    try:
        main(args)
    except KeyboardInterrupt:
        logging.critical("Terminated by user")
        sys.exit(1)
    except Exception as exc:
        logging.critical("Build failure due to %s", exc)
        if args.verbose:
            raise exc
        sys.exit(1)


if __name__ == "__main__":
    launch()
