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
import re
import shutil
import subprocess
import sys
from pathlib import Path

import oauth2client.client
from aemu.android_build_client import AndroidBuildClient
from aemu.fetch_artifact import unzip_artifact
from aemu.log import configure_logging
from tqdm import tqdm


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


def git_commit(destination_dir, args):
    """Creates a Git commit with a message summarizing the bazel build update.

    Args:
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
    commit_header = f"Update bazel build from http://aosp/{args.cl}"

    # Add a commit_msg footer if buganizer_id is specified
    commit_footer = f"\nBug: {args.buganizer_id}" if args.buganizer_id else ""

    commit_msg = f"""

{commit_header}

This updates the bazel build from the running the meson generator on build bots.

The build is based on http://aosp/{args.cl}, with presubmit build targets:

- go/ab/{args.mac_bid}
- go/ab/{args.win_bid}
- go/ab/{args.lin_bid}

You can recreate the commit by running:

```sh
            {sys.argv[0]} \\
            --dest {args.dest} \\
            --cl {args.cl} \\
            --mac-bid {args.mac_bid} \\
            --win-bid {args.win_bid} \\
            --lin-bid {args.lin_bid} \\
            --re {args.reviewers} \\

```
{commit_footer}
            """

    run(
        ["git", "-C", str(destination_dir), "commit", "-m", commit_msg],
        dry_run=args.dry_run,
    )


def obtain(args, target: str, platform: str, bid: str, dest: Path):
    """Obtains a specified target for a given build and integrates it into a Git repository.

    Args:
        args: An argparse.Namespace object containing the following arguments:
            - token: The authentication token for the Android Build Client.
        bid: The Build ID (BID) of the target to obtain.
        target: The name of the target to obtain.
        platform: The platform of interest
        dest: The directory where we should unzip this
    """
    logging.info("Obtaining target %s from build: %s", target, bid)
    ab_client = AndroidBuildClient(args.token)

    artifact = f"bazel-{platform}-{bid}.zip"
    unzip_artifact(
        ab_client,
        dest,
        target,
        artifact,
        bid,
    )


def fix_up_prefix(dest_dir: Path):
    """
    Removes occurrences of build-specific prefixes from files within a directory.

    This function first identifies uniqueprefix paths defined in 'config-host.h'
    files within the specified directory. It then finds all non-hidden files in the
    directory and replaces any occurrence of these prefixes with the string "rel_dir".

    Note: This is *a good enough* solution for the few header files we have.

    Args:
        dest_dir (Path): The directory in which to search for and modify files.

    Returns:
        None

    Raises:
        FileNotFoundError: If the specified directory or any files within it
                           cannot be found.
        PermissionError: If there are insufficient permissions to read or write files.
        IOError: If there's a general error during file operations.
    """

    prefix_paths = []
    config_files = dest_dir.rglob("config-host.h")

    for config_file in config_files:
        with open(config_file, "r") as f:
            for line in f:
                match = re.search(r'^#define\s+CONFIG_PREFIX\s+"([^"]+)"', line.strip())
                if match:
                    unique_prefix_path = match.group(1)
                    prefix_paths.append(unique_prefix_path)
                    break

    all_files = dest_dir.rglob("*")
    for file_path in tqdm(all_files):
        if file_path.is_file():
            with open(file_path, "r", encoding="utf-8") as f:
                file_contents = f.read()

                # Track if any changes were made to this file
                file_modified = False
                for prefix_path in prefix_paths:
                    if prefix_path in file_contents:
                        file_contents = file_contents.replace(prefix_path, "rel_dir")
                        file_modified = True

                # Only write back to the file if it was modified
                if file_modified:
                    with open(file_path, "w") as f:
                        f.write(file_contents)


def fetch_and_merge(args):
    targets = {
        "emulator-windows_x64": ("windows-amd64", args.win_bid),
        "emulator-mac_aarch64": ("darwin-arm64", args.mac_bid),
        "emulator-linux_x64": ("linux-x86_64", args.lin_bid),
    }

    dest = Path(args.dest)
    dest.mkdir(parents=True, exist_ok=True)

    # Obtain all the pieces
    for target, (platform, bid) in targets.items():
        obtain(args, target, platform, bid, dest)

    # Next merge them all together
    qemu = Path(args.aosp) / "external" / "qemu"
    merge = qemu / "google" / "scripts" / "merge_bazel.py"

    merge_cmd = [
        sys.executable,
        str(merge),
        "--buildfile",
        str(dest / "platform" / "BUILD.windows-amd64"),
        "@platforms//os:windows",
        "--buildfile",
        str(dest / "platform" / "BUILD.darwin-arm64"),
        "@platforms//os:macos",
        "--buildfile",
        str(dest / "platform" / "BUILD.linux-x86_64"),
        "@platforms//os:linux",
        "--verbatim-buildfile",
        str(qemu / "platform" / "BUILD.common"),
        "--unique",
        "win_def_file",
        "-o",
        str(dest / "BUILD.bazel"),
    ]
    logging.info("Merging using: %s", " ".join(merge_cmd))
    subprocess.check_call(merge_cmd)
    logging.info("Running buildifier on %s", str(dest / "BUILD.bazel"))
    subprocess.check_call(["buildifier", str(dest / "BUILD.bazel")])

    # Let's remove all build bot specific information.
    fix_up_prefix(dest / "platform")

    run(["git", "-C", str(dest), "add", "*"], dry_run=args.dry_run)


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
        This tool can be used to fetch the generated bazel files from the meson generator. The tool expects you to have
        ~/go/bin/oauth2l installed. You can find details on how to install this tool here: http://go/oauth2l



        Note: Token generation is only supported on glinux. For mac you will manually have to create a token on your glinux machine by running:

        ~/go/bin/oauth2l fetch --sso $USER@google.com androidbuild.internal
        """,
        epilog="""Examples:

        fetch-bazel  --cl 306320 --mac-bid P74111280  --win-bid P74150599 --lin-bid P73982148
        """,
    )
    parser.add_argument(
        "--dest",
        type=str,
        default=find_aosp_root() / "external" / "qemu",
        help="Destination path for the final BUILD.bazel file (usually $AOSP/external/qemu)",
    )
    parser.add_argument(
        "--aosp",
        type=str,
        default=find_aosp_root(),
        help="AOSP root",
    )
    parser.add_argument(
        "--cl",
        type=str,
        required=True,
        help="The gerrit cl used to generate these headers.",
    )
    parser.add_argument(
        "--token",
        type=str,
        help="Apiary OAuth2 token. If not set we will try to obtain one",
    )
    parser.add_argument(
        "--win-bid",
        type=str,
        required=True,
        help="Windows build from which we will fetch bazel-windows-amd64-{win-bid}.zip ",
    )
    parser.add_argument(
        "--lin-bid",
        type=str,
        required=True,
        help="Linux build from which we will fetch bazel-linux-x86_64-{lin-bid}.zip ",
    )
    parser.add_argument(
        "--mac-bid",
        type=str,
        required=True,
        help="Darwin build from which we will fetch bazel-darwin-arm64-{mac-bid}.zip ",
    )
    parser.add_argument(
        "--no-repo", action="store_true", help="Do not use any repo commands"
    )

    parser.add_argument(
        "--re",
        default="yahan@google.com,jansene@google.com,bohu@google.com,jpcottin@google.com",
        dest="reviewers",
        type=str,
        help="Set of reviewers for the gerrit change that will be created.",
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
        help="Include Buganizer ID in the gerrit commit description ",
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
    try:
        if not args.no_repo:
            run(
                ["repo", "start", f"update-bazel-{args.cl}"],
                dry_run=args.dry_run,
                cwd=destination_dir,
            )

        fetch_and_merge(args)

        git_commit(destination_dir, args)

        if not args.no_repo:
            logging.info("Created change.. Uploading to gerrit")
            run(
                [
                    "repo",
                    "upload",
                    "-y",
                    "--label",
                    "Presubmit-Ready+1",
                    f"--br=update-bazel-{args.cl}",
                    f"--re={args.reviewers}",
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
        if not args.no_repo:
            run(
                ["repo", "abandon", f"update-bazel-{args.cl}"],
                dry_run=args.dry_run,
                cwd=destination_dir,
            )


if __name__ == "__main__":
    main()
