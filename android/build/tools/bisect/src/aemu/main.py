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


def do_bisect(args, destination_dir):
    ab_client = AndroidBuildClient(args.token)

    def run_shell_cmd(bid):
        return invoke_shell_with_artifact(
            ab_client, destination_dir, args.build_target, args.artifact, args.cmd, bid
        )

    build_ids = ab_client.list_builds(
        args.branch, args.build_target, args.bid, args.num
    )
    bisect(build_ids, run_shell_cmd)


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
        "--num",
        type=int,
        default=40,
        help="Max number of build ids to check",
    )
    parser.add_argument(
        "--bid", type=str, help="Starting build id to check, or None to use the latest"
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
        "cmd",
        type=str,
        help="Command to execute for the given build. Information about the build is passed through environment variables.",
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
