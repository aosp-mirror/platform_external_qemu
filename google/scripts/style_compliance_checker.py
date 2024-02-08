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
#
#
# This script compares files with a git tag, ignoring CONFIG_ANDROID blocks.
# It helps to minimize difficulties during merging by ensuring all android code changes
# are placed between #ifdef CONFIG_ANDROID and #endif blocks. This helps to group
# all android changes together and prevents them from leaking into the QEMU codebase
# after merging.
import argparse
import re
import subprocess
import logging
from pathlib import Path
from typing import List


class CustomFormatter(
    argparse.RawTextHelpFormatter, argparse.ArgumentDefaultsHelpFormatter
):
    pass


def find_files_with_string(start_dir: Path, string: str) -> List[Path]:
    """
      Recursively finds all .c and .h files in a directory and its subdirectories
      that contain a specific string. Stops reading after first match.


    Args:
      start_dir: The directory to start searching from (Path object).
      string: The string to search for in the files.

    Returns:
      A list of all files (Path objects) that contain the string.
    """
    files = []
    logging.info("Searching %s for files", start_dir)
    for path in start_dir.glob("**/*.[ch]"):
        logging.debug("Inspecting %s", path)
        if path.is_file():
            with path.open("r") as f:
                for line in f:
                    if string in line:
                        files.append(path)
                        break
    return files


def remove_ifdef_blocks(text: str) -> str:
    """Removes #ifdef CONFIG_ANDROID and #endif blocks and all the text in between.

    For example

    void foo() {
        int x;
    #ifdef CONFIG_ANDROID
        // do this
    #else
        // do that
    #endif
    }

    Becomes

    void foo() {
        int x;
        // do that
    }
    """
    res = ""
    keep = True
    config_android = False
    config_android_ifdef = re.compile("\s*#\s*ifdef\s+CONFIG_ANDROID\s*")
    config_android_else = re.compile("\s*#\s*else")
    config_android_endif = re.compile("\s*#\s*endif")

    for line in text.splitlines(keepends=True):
        if config_android_ifdef.match(line):
            keep = False
            config_android = True

        if config_android:
            if config_android_else.match(line):
                keep = True
                continue

            if config_android_endif.match(line):
                config_android = False
                keep = True
                continue

        if keep:
            res += line

    return res


def compare_texts(text1: str, text2: str) -> bool:
    """
    Compares two texts and returns True if they are equal, ignoring changes where lines are all blank.

    Args:
        text1: The first text to compare.
        text2: The second text to compare.

    Returns:
        True if the texts are equal, ignoring blank lines, False otherwise.
    """

    lines1 = [x for x in text1.splitlines() if x.strip()]
    lines2 = [x for x in text2.splitlines() if x.strip()]

    # Track line numbers for mismatch reporting
    line_num = 1

    # Ignore blank lines
    for line1, line2 in zip(lines1, lines2):
        stripped_line1 = line1.strip()
        stripped_line2 = line2.strip()

        if stripped_line1 != stripped_line2:
            print(f"Line {line_num}: Mismatch found\n{line1}\n{line2}")
            return False

        line_num += 1

    # Check if reached the end of one file before the other
    if len(lines1) != len(lines2):
        remaining_lines = lines1 if len(lines1) > len(lines2) else lines2
        print(f"Line {line_num}: One text has additional lines:\n{remaining_lines[0]}")
        return False

    return True


def find_aosp_root(start_directory: Path = Path(__file__).resolve()) -> Path:
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
        description="""Compare files with git tag, ignoring CONFIG_ANDROID blocks

To minimize the difficulties during merging we adopt the policy that all android
code changes are placed between

#ifdef CONFIG_ANDROID
    .. android only code ..
#endif

This will make sure all android changes are grouped and do not accidently leak
out into the QEMU code base after merging.
        """,
        formatter_class=CustomFormatter,
    )
    parser.add_argument(
        "directory",
        default=".",
        help="Search the given directory for .c and .h files with CONFIG_ANDROID blocks",
    )
    parser.add_argument(
        "--file-tag",
        nargs=2,
        default=[],
        action="append",
        help="File-tag pair indicating which tag should be used to compare the given file against.",
    )
    parser.add_argument(
        "--aosp",
        type=str,
        default=find_aosp_root(),
        help="AOSP root to use",
    )
    parser.add_argument(
        "--exclude",
        default=[],
        action="append",
        help="Files that should be excluded from the comparison.",
    )
    parser.add_argument(
        "--tag",
        # Update this default tag if you do another git merge.
        default="1d675e59ea",
        help="Default tag compare a file against",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Enable verbose logging"
    )

    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
    file_tags = dict(args.file_tag)
    start_dir = Path(args.directory).absolute()
    here = Path(args.aosp, "external", "qemu").absolute()

    for absolute_file in find_files_with_string(start_dir, "CONFIG_ANDROID"):
        filename = absolute_file.relative_to(start_dir)

        # Find tag against we should compare.
        tag = file_tags.get(str(filename), args.tag)
        if (
            str(filename) in args.exclude
            or subprocess.run(
                ["git", "-C", here, "cat-file", "-e", f"{tag}:{filename}"],
                capture_output=True,
            ).returncode
            != 0
        ):
            logging.debug("File %s, does not exist at %s (that's ok)", filename, tag)
            continue

        # Get git tag content
        tag_content = subprocess.run(
            ["git", "-C", here, "show", f"{tag}:{filename}"], capture_output=True
        ).stdout.decode()

        # Read source file content and remove CONFIG_ANDROID blocks
        with open(absolute_file) as f:
            source_content = f.read()
        cleaned_content = remove_ifdef_blocks(source_content)

        # Compare contents and warn if different
        if not compare_texts(cleaned_content, tag_content):
            logging.fatal("File '%s' differs from git tag '%s'.", filename, tag)
            logging.fatal(
                "All android specific code changes need to be between an `#ifdef CONFIG_ANDROID ... #endif` block"
            )
            exit(1)
        else:
            logging.info(f"File '%s' matches git commit '%s'.", filename, tag)


if __name__ == "__main__":
    main()
