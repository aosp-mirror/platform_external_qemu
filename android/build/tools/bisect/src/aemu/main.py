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
from aemu.android_build_client import AndroidBuildClient
from aemu.bisect import bisect
from aemu.fetch_artifact import invoke_shell_with_artifact
from aemu.log import configure_logging

import argparse
import logging
import oauth2client.client
from pathlib import Path
import sys


class CustomFormatter(
    argparse.RawTextHelpFormatter, argparse.ArgumentDefaultsHelpFormatter
):
    pass


def find_builds_in_range(ab_client, branch, target, start, end, num=8192):
    if end and start < end:
        raise ValueError(
            f"Starting build {start} should be higher than the end build {end}, maybe swap them?"
        )
    # So the build api does [start, end> meaning that the end
    # will not be included, so we are first going to find the build
    # that comes after end.
    build_end = int(end) - 1 if end else 100

    # Now we are going to grow our set, until we have the build range
    # we want to work with, note we assume start exists.. We will error
    # out later on if it does not
    build_ids = {start}
    last_size = 0
    while ((end and end not in build_ids) or len(build_ids) < num) and last_size != len(
        build_ids
    ):
        last_size = len(build_ids)
        start = min(build_ids)
        build_ids.update(
            [
                int(x)
                for x in ab_client.list_builds(branch, target, start, build_end, 512)
            ]
            or []
        )
        logging.debug("Grew set from %s to %s", last_size, len(build_ids))

    if end and end not in build_ids:
        raise ValueError(f"Unable to find build {end}")

    logging.debug("Found [%s]", ",".join([str(x) for x in build_ids]))
    return build_ids


def do_bisect(args, destination_dir):
    ab_client = AndroidBuildClient(args.token)

    def run_shell_cmd(bid):
        return invoke_shell_with_artifact(
            ab_client,
            destination_dir,
            args.build_target,
            args.artifact,
            args.cmd,
            bid,
            args.unzip,
            args.overwrite,
        )

    start = args.start
    if not start:
        start = max(args.good, args.bad)

    if args.good or args.bad:
        end = min(args.good, args.bad)
    else:
        end = args.end

    if end:
        build_ids = find_builds_in_range(
            ab_client, args.branch, args.build_target, start, end
        )
    else:
        build_ids = find_builds_in_range(
            ab_client, args.branch, args.build_target, start, None, args.num
        )

    if not build_ids or len(build_ids) < 2:
        raise ValueError(
            f"Did not retrieve enough build ids to bisect, retrieved: {build_ids}"
        )

    build_ids = sorted(build_ids, reverse=True)
    bisect(build_ids, run_shell_cmd, good=args.good, bad=args.bad)


def main():
    parser = argparse.ArgumentParser(
        formatter_class=CustomFormatter,
        description="""
        bisect uses binary search to find a pair of adjacent builds in the given range such that the given shell command succeeds on one build and fails on the other.
        works with any shell command that communicates success or failure via a zero/non-zero exit status.

        Information about the build under consideration is passed in through environment variables:

        - `ARTIFACT_UNZIP_DIR`: variable containing the directory of the unzipped artifact, if the artifact was a zip file.
        - `ARTIFACT_LOCATION` variable containling the location of the downloaded artifact.
        - `X` the current build id under consideration.

        Note: Token generation is only supported on glinux. For mac and windows you will manually have to create a token on your glinux machine by running:

        ~/go/bin/oauth2l fetch --sso $USER@google.com androidbuild.internal
        """,
        epilog="""Examples:

        Finding a UI issue in linux, you will have the respond with y/n after invocation.

        emu-bisect --num 1024 '${ARTIFACT_UNZIP_DIR}/emulator/emulator @my_avd -qt-hide-window -no-snapshot -grpc-use--token; read -p "Is $X OK? (y/n): " ok < /dev/tty;  [[ "$ok" == "y" ]]'

        Example invocation on Mac OS, where the environment variable TOKEN contains a valid oauth token.

        emu-bisect --num 2 --artifact sdk-repo-darwin_aarch64-emulator-{bid}.zip --build_target emulator-mac_aarch64_gfxstream  --token $TOKEN  'read -p "Is $X OK? (y/n): " ok < /dev/tty;  [[ "$ok" == "y" ]]'

        If your token expires halfway during the bisect (if it is long running) you can restart it where you left off by using the --good, and --bad flags.

        For example you might see this on the log

        bisect: iteration 7 of 9 or 10, 10308137, not ok - (10289643) - 10271065, ok

        You can now continue by:

        emu-bisect --bad 10308137 --good 10271065

        """,
    )
    parser.add_argument(
        "--dest",
        type=str,
        default="/tmp/emu-bisect",
        help="Destination directory or file where we place the downloaded artifacts",
    )
    parser.add_argument(
        "--token",
        type=str,
        help="Apiary OAuth2 token. If not set we will try to obtain one",
    )
    parser.add_argument(
        "--branch",
        default="aosp-emu-master-dev",
        type=str,
        help="go/ab branch",
    )
    parser.add_argument(
        "--start",
        type=int,
        help="Starting build id to check, or None to use the latest",
    )
    parser.add_argument(
        "--end", type=int, help="Ending build id to check, omit to only use --num"
    )
    parser.add_argument(
        "--good", type=int, help="Build id to mark as good, or None to detect"
    )
    parser.add_argument(
        "--bad", type=int, help="Build id to mark as bad, or None to detect"
    )
    parser.add_argument(
        "--num",
        type=int,
        default=1024,
        help="Max number of build ids to check",
    )

    parser.add_argument(
        "--build_target",
        default="emulator-linux_x64_gfxstream",
        type=str,
        help="go/ab build target",
    )
    parser.add_argument(
        "--artifact",
        default="sdk-repo-linux-emulator-{bid}.zip",
        type=str,
        help="go/ab target artifact. The string {bid} will be replaced by the build id.",
    )
    parser.add_argument(
        "--unzip",
        default=False,
        type=str,
        help="Unzip the obtained artifact.",
    )
    parser.add_argument(
        "cmd",
        type=str,
        help="Command to execute for the given build. Information about the build is passed through environment variables.",
    )
    parser.add_argument(
        "-o",
        "--overwrite",
        dest="overwrite",
        default=False,
        action="store_true",
        help="Overwrite previously downloaded artifact",
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

    if args.good and (args.start or args.end):
        raise ValueError("--good cannot be used with start and end.")

    if args.bad and (args.start or args.end):
        raise ValueError("--bad cannot be used with start and end.")

    lvl = logging.DEBUG if args.verbose else logging.INFO
    configure_logging(lvl)

    # Make sure we have a destination that works
    destination_dir = Path(args.dest)
    if destination_dir.exists() and not destination_dir.is_dir():
        logging.fatal("Destination %s exists and is not a directory", destination_dir)
        sys.exit(1)
    destination_dir.mkdir(parents=True, exist_ok=True)

    # TODO(jansene): You could add support for multitude of commands, versus just bisect
    # such as:
    # for_each build execute a command on an artifacts (say reprocess symbols?)
    # fetch a single build
    # etc..

    try:
        do_bisect(args, destination_dir)
    except oauth2client.client.AccessTokenCredentialsError as err:
        logging.fatal(
            "Token failure. You might have to clear your cache with ~/go/bin/oauth2l reset. (%s)",
            err,
        )
    except ValueError as err:
        logging.fatal("%s", err)


if __name__ == "__main__":
    main()
