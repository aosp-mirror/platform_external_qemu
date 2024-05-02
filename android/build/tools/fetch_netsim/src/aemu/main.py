#!/usr/bin/env python3
#
# Copyright 2024 - The Android Open Source Project
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
import shutil
import subprocess
import sys
from pathlib import Path

import oauth2client.client
from aemu.android_build_client import AndroidBuildClient
from aemu.fetch_artifact import unzip_artifact
from aemu.log import configure_logging


class CustomFormatter(
    argparse.RawTextHelpFormatter, argparse.ArgumentDefaultsHelpFormatter
):
    pass


TARGET_MAP = {
    "windows_x64": "windows_msvc-x86_64",
    "linux_x64": "linux-x86_64",
    "mac_x64": "darwin-x86_64",
    "mac_aarch64": "darwin-aarch64",
}


def run(cmd, dry_run: bool, cwd: Path = None):
    """Executes a command in a specified working directory.

    Args:
        cmd: A list of strings representing the command to execute.
        cwd: An optional Path object representing the working directory. If
            None, the current working directory is used.

    Raises:
        subprocess.CalledProcessError: If the command exits with a non-zero
            return code.
    """
    if cwd:
        cwd = Path(cwd).absolute()

    cmd = [str(c) for c in cmd]
    if dry_run:
        logging.warning("Dry run! Skipping: %s", " ".join(cmd))
        return

    logging.warning("Running: %s in %s", " ".join(cmd), cwd)
    subprocess.run(
        cmd,
        cwd=cwd,
    )


def git_commit(bid, destination_dir, args):
    """Creates a Git commit with a message summarizing the netsim update.

    Args:
        bid: The Build ID (BID) of the netsim update.
        destination_dir: The git directory where we are going to commit
        args: An argparse.Namespace object containing additional arguments:
            - dest: The destination directory for the update.
            - branch: The Git branch to commit to.
            - prefix: The prefix for the commit message.
            - targets: The target files for the update.
            - reviewers: The reviewers for the commit.
            - artifact: The artifact to commit.
    """
    # Append to the header if netsim_version and canary_version are specified
    commit_header = f"Update netsim to {bid}"
    if args.netsim_version and args.canary_version:
        commit_header += f" (Netsim {args.netsim_version} for Canary {args.canary_version})"

    # Add a commit_msg footer if buganizer_id is specified
    commit_footer = f"\nBug: {args.buganizer_id}" if args.buganizer_id else ""

    commit_msg = f"""

{commit_header}

This updates netsim with the artifacts from go/ab/{bid}.
You can recreate the commit by running:

```sh
            {sys.argv[0]} \\
            --bid {bid} \\
            --dest {args.dest} \\
            --branch {args.branch} \\
            --prefix {args.prefix} \\
            --targets {args.targets} \\
            --re {args.reviewers} \\
            --artifact {args.artifact}
```
{commit_footer}
            """

    run(
        ["git", "-C", str(destination_dir), "commit", "-m", commit_msg],
        dry_run=args.dry_run,
    )


def get_bid(args):
    """Retrieves the Build ID (BID) to use for the netsim update.

    Args:
        args: An argparse.Namespace object containing the following arguments:
            - bid: An optional BID provided directly. If specified, it takes
                precedence.
            - token: The authentication token for the Android Build Client.
            - branch: The branch to query for the latest BID.
            - prefix: The prefix for the build name to filter by.

    Returns:
        The Build ID (BID) to use for the update.
    """
    if args.bid:
        return args.bid

    ab_client = AndroidBuildClient(args.token)
    return ab_client.get_latest_build_id(args.branch, f"{args.prefix}-linux_x64")


def obtain(args, bid: str, target: str, base_dir: Path):
    """Obtains a specified target for a given build and integrates it into a Git repository.

    Args:
        args: An argparse.Namespace object containing the following arguments:
            - token: The authentication token for the Android Build Client.
            - artifact: The artifact format string (to be formatted with bid and target).
            - prefix: The prefix for the build name.
        bid: The Build ID (BID) of the target to obtain.
        target: The name of the target to obtain.
        base_dir: The base directory where the netsim prebuilt should live.
    """
    logging.info("Obtaining target %s from build: %s", target, bid)
    ab_client = AndroidBuildClient(args.token)

    artifact = args.artifact.format(bid=bid, target=TARGET_MAP[target])

    dest = base_dir / TARGET_MAP[target]
    if dest.exists():
        logging.warning("Removing existing directory %s", dest)
        shutil.rmtree(dest)
        run(
            ["git", "-C", str(base_dir), "rm", "-rf", TARGET_MAP[target]],
            dry_run=args.dry_run,
        )

    dest.mkdir(parents=True, exist_ok=True)
    unzip_artifact(
        ab_client,
        dest,
        f"{args.prefix}-{target}",
        artifact,
        bid,
    )
    run(["git", "-C", str(base_dir), "add", TARGET_MAP[target]], dry_run=args.dry_run)


def find_aosp_root(start_directory=Path(__file__).resolve()) -> Path:
    current_directory = Path(start_directory).resolve()

    while True:
        repo_directory = current_directory / ".repo"
        if repo_directory.is_dir():
            return current_directory

        # Move up one directory
        parent_directory = current_directory.parent

        # Check if we've reached the root directory
        if current_directory == parent_directory:
            return str(current_directory)

        current_directory = parent_directory


def main():
    parser = argparse.ArgumentParser(
        formatter_class=CustomFormatter,
        description="""
        This tool can be used to fetch the latest netsim build and unzip it to the proper directory. The tool expects you to have
        ~/go/bin/oauth2l installed. You can find details on how to install this tool here: http://go/oauth2l



        Note: Token generation is only supported on glinux. For mac you will manually have to create a token on your glinux machine by running:

        ~/go/bin/oauth2l fetch --sso $USER@google.com androidbuild.internal
        """,
        epilog="""Examples:

        fetch-netsim --bid 11299776
        fetch-netsim --re foo@bar,bar@foo
        """,
    )
    parser.add_argument(
        "--dest",
        type=str,
        default=find_aosp_root()
        / "prebuilts"
        / "android-emulator-build"
        / "common"
        / "netsim",
        help="Destination directory for the netsim prebuilt, usually this is in AOSP",
    )
    parser.add_argument(
        "--token",
        type=str,
        help="Apiary OAuth2 token. If not set we will try to obtain one",
    )
    parser.add_argument(
        "--branch",
        default="aosp-netsim-dev",
        type=str,
        help="go/ab branch",
    )
    parser.add_argument(
        "--bid",
        type=int,
        help="Starting build id to check, or None to use the latest",
    )
    parser.add_argument(
        "--prefix",
        default="emulator",
        help="Target prefix. This is the bold header you see in go/ab",
    )

    parser.add_argument(
        "--targets",
        default="linux_x64,mac_x64,mac_aarch64,windows_x64",
        type=str,
        help="go/ab build target",
    )
    parser.add_argument(
        "--artifact",
        default="netsim-{target}-{bid}.zip",
        type=str,
        help="go/ab target artifact. The string {bid} will be replaced by the build id, and target by build_target. Usually no change is needed.",
    )
    parser.add_argument(
        "--re",
        default="formosa@google.com,schilit@google.com,bohu@google.com,jpcottin@google.com",
        dest="reviewers",
        type=str,
        help="Set of reviewers for the gerrit change that will be created.",
    )
    parser.add_argument(
        "--cc",
        default="jansene@google.com,shuohsu@google.com,hyunjaemoon@google.com",
        dest="cc",
        type=str,
        help="Set of ccs for the gerrit change that will be created.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Dry run, only obtain artifacts, do not use git or repo.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        default=False,
        action="store_true",
        help="Verbose logging",
    )
    parser.add_argument(
        "--buganizer-id",
        type=str,
        help="Include Buganizer ID in the gerrit commit description "
    )
    parser.add_argument(
        "--netsim-version",
        type=str,
        help="Include Netsim version in the gerrit commit description"
    )
    parser.add_argument(
        "--canary-version",
        type=str,
        help="Include target Canary version for Netsim distribution in the gerrit commit description"
    )

    args = parser.parse_args()

    lvl = logging.DEBUG if args.verbose else logging.INFO
    configure_logging(lvl)

    # Make sure we have a destination that works
    destination_dir = Path(args.dest)
    if destination_dir.exists() and not destination_dir.is_dir():
        logging.fatal("Destination %s exists and is not a directory", destination_dir)
        sys.exit(1)

    logging.info("Preparing %s", destination_dir)
    destination_dir.mkdir(parents=True, exist_ok=True)
    bid = None
    try:
        bid = get_bid(args)
        run(
            ["repo", "start", f"netsim-{bid}"],
            dry_run=args.dry_run,
            cwd=destination_dir,
        )
        for target in args.targets.split(","):
            obtain(args, bid, target.strip(), destination_dir)
        git_commit(bid, destination_dir, args)

        logging.info("Created change.. Uploading to gerrit")
        run(
            [
                "repo",
                "upload",
                "-y",
                "--label",
                "Presubmit-Ready+1",
                f"--br=netsim-{bid}",
                f"--re={args.reviewers}",
                f"--cc={args.cc}",
                str(destination_dir),
            ],
            dry_run=args.dry_run,
            cwd=destination_dir,
        )
    except oauth2client.client.AccessTokenCredentialsError as err:
        logging.fatal(
            "Token failure. You might have to clear your cache with ~/go/bin/oauth2l reset. (%s)",
            err,
        )
    except ValueError as err:
        logging.fatal("%s", err)
    finally:
        # Clean up repo branch if any
        if bid:
            run(
                ["repo", "abandon", f"netsim-{bid}"],
                dry_run=args.dry_run,
                cwd=destination_dir,
            )


if __name__ == "__main__":
    main()
