#!/usr/bin/env python
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
from pathlib import Path
from collections import namedtuple
import logging
import shutil
import sys

Module = namedtuple("MODULE", "operatingsystem architecture id name")


def parse_breakpad_line(line):
    """Parses a breakpad symbol line and returns a concrete object contained in a line.

    See: https://chromium.googlesource.com/breakpad/breakpad/+/refs/heads/main/docs/symbol_files.md

    Args:
        line (str): Line from the breakpad symbol file

    Returns:
        Module: A parsed module object
    """
    value = [x.strip() for x in line.split(" ")]
    if value[0] == "MODULE":
        return Module(*value[1:])


def relocate_symbol_file(symbol_file, destination_directory, copy):
    """Copies a symbol file to a location where the resolver will find it.

    This is destination / module.name / module.id / module.name.sym

    Args:
        symbol_file (Path): The breakpad symbol file
        destination_directory (Path): The destination directory.
        copy (bool): True if the file should be copied, v.s. moved.
    """
    with open(symbol_file, "r", encoding="utf-8") as symbol:
        module = parse_breakpad_line(symbol.readline())


    destination = destination_directory / module.name / module.id
    destination.mkdir(exist_ok=True, parents=True)
    destination_sym = (destination / module.name).with_suffix(".sym")

    assert destination.exists(), f"Directory creation of {destination} did not succeed."


    if destination_sym.exists():
        logging.warning("Destination symbol %s already exists, deleting", destination_sym)
        destination_sym.unlink(missing_ok=True)

    if copy:
        logging.info("Copying %s -> %s", symbol_file, destination_sym)
        shutil.copyfile(symbol_file, destination_sym)
    else:
        logging.info("Moving %s -> %s", symbol_file, destination_sym)
        shutil.move(symbol_file, destination_sym)

    assert destination_sym.exists(), f"Expecting {destination_sym} to exist!"


def recursively_relocate_symbols(src, dest, copy):
    """Recursivley copy the symbol files in the src path to the destination directory.

    Args:
        src (Path): The directory to recusively scan for symbol files.
        dest (Path): The destination of the symbol files.
        copy (bool): True if the file should be copied, v.s. moved.
    """
    for maybe_symbol_file in src.rglob("*"):
        logging.debug("Processing %s", maybe_symbol_file)
        if is_symbol_file(maybe_symbol_file):
            relocate_symbol_file(maybe_symbol_file, dest, copy)


def is_symbol_file(symbol_file):
    """True if this is a breakpad symbol file

    Args:
        symbol_file (Path) to test

    Returns:
        bool: True if this is a readable breakpad symbol file
    """
    if not symbol_file.exists() or symbol_file.is_dir():
        return False

    try:
        with open(symbol_file, "r", encoding="utf-8") as symbol:
            module = parse_breakpad_line(symbol.readline())
            return module is not None
    except Exception as exc:
        logging.debug(
            "Unable to validate if %s is symbol file", symbol_file, exc_info=exc
        )
        return False


def is_symbol_file_or_directory(param):
    """Checks to see if this parameter is a symbol file or directory.

    Args:
        param (str): The parameter to be validated.

    Raises:
        argparse.ArgumentTypeError: Not a directory or symbol file.

    Returns:
        Path: A path that can be used.
    """
    sym = Path(param)
    if not sym.exists():
        raise argparse.ArgumentTypeError(param + " does not exist.")

    if sym.is_file() and not is_symbol_file(sym):
        raise argparse.ArgumentTypeError(param + " is not a valid symbol file.")

    return sym


def configure_logging(logging_level):
    """Configures the logging system to log at the given level

    Args:
        logging_level (_type_): A logging level, or number.
    """
    logging_handler_out = logging.StreamHandler(sys.stdout)
    logging.root.setLevel(logging_level)
    logging.root.addHandler(logging_handler_out)


def main():
    parser = argparse.ArgumentParser(
        prog="ProgramName",
        description="Relocate the breakpad symbol file(s) to a location that can be resolved.",
    )
    parser.add_argument(
        "symbol_file",
        metavar="symbol",
        type=is_symbol_file_or_directory,
        nargs="+",
        help="symbol file or directory to process.",
    )
    parser.add_argument(
        "-r",
        "--recursive",
        action="store_true",
        default=False,
        help="Recursively process files",
    )
    parser.add_argument(
        "-c",
        "--copy",
        action="store_true",
        default=False,
        help="Copy the files instead of moving them",
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
        "-o",
        "--out",
        default=Path("objs") / "build" / "symbols",
        help="Directory where the symbol files should be placed",
    )

    args = parser.parse_args()
    lvl = logging.DEBUG if args.verbose else logging.INFO
    configure_logging(lvl)

    for path in args.symbol_file:
        if path.is_file():
            relocate_symbol_file(path, Path(args.out), args.copy)
        if path.is_dir() and args.recursive:
            recursively_relocate_symbols(path, Path(args.out), args.copy)


if __name__ == "__main__":
    main()
