import subprocess
import sys
import os
import argparse
import tempfile
from pathlib import Path
from typing import List

DEFAULT_LLVM_ROOT = (
    Path(__file__).parents[7]
    / "prebuilts"
    / "clang"
    / "host"
    / "windows-x86"
    / "clang-r487747c"
    / "bin"
)


def extract_objects(archive_file: str, output_directory: str, llvm_root: Path) -> None:
    """
    Extract objects from the archive using llvm-ar.

    Parameters:
        archive_file (str): Path to the archive file.
        output_directory (str): Path to the output directory.
        llvm_root (Path): Path to the LLVM root directory.

    Raises:
        subprocess.CalledProcessError: If the extraction process fails.
    """
    subprocess.run(
        [llvm_root / "llvm-ar", "x", archive_file], cwd=output_directory, check=True
    )


def combine_objects(object_directory: str, output_lib: str, llvm_root: Path) -> None:
    """
    Combine object files into an MSVC-compatible library using llvm-lib.

    Parameters:
        object_directory (str): Path to the directory containing object files.
        output_lib (str): Path to the output library file.
        llvm_root (Path): Path to the LLVM root directory.

    Raises:
        subprocess.CalledProcessError: If the library generation process fails.
    """
    object_files = [
        Path(object_directory) / file
        for file in os.listdir(object_directory)
        if file.endswith(".o")
    ]

    cmd = [
        str(x)
        for x in [llvm_root / "llvm-lib"]
        + object_files
        + [f"/OUT:{Path(output_lib).absolute()}"]
    ]
    print(" ".join(cmd))
    subprocess.run(
        cmd,
        cwd=object_directory,
        check=True,
    )


def convert_mingw_to_msvc_lib(
    archive_file: Path, output_lib: Path, llvm_root: Path
) -> None:
    """
    Convert a MinGW archive to an MSVC-compatible library.

    Parameters:
        archive_file (str): Path to the archive file.
        output_lib (str): Path to the output library file.
        llvm_root (Path): Path to the LLVM root directory.
    """
    try:
        with tempfile.TemporaryDirectory() as temp_dir:
            # Extract the individual object files from the archive
            extract_objects(archive_file, temp_dir, llvm_root)

            # Combine the object files into an MSVC-compatible library
            combine_objects(temp_dir, output_lib, llvm_root)

        print(f"Successfully converted {archive_file} to {output_lib}")
    except subprocess.CalledProcessError as e:
        print("Conversion failed:", e)
    except Exception as e:
        print("Error occurred:", e)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert MinGW archive to MSVC-compatible library."
    )
    parser.add_argument("archive_file", type=str, help="Path to the archive file.")
    parser.add_argument("output_lib", type=str, help="Path to the output library file.")
    parser.add_argument(
        "--llvm-root",
        type=str,
        default=DEFAULT_LLVM_ROOT,
        help="Path to the LLVM root directory.",
    )
    parser.add_argument(
        "-r",
        "--recursive",
        action="store_true",
        help="Process all archives, versus a single one",
    )
    args = parser.parse_args()

    try:
        if args.recursive:
            for archive in Path(args.archive_file).glob("*.a"):
                convert_mingw_to_msvc_lib(
                    archive,
                    Path(args.output.lib) / archive.with_suffix(".lib").name,
                    Path(args.llvm_root),
                )
        else:
            convert_mingw_to_msvc_lib(
                Path(args.archive_file), Path(args.output_lib), Path(args.llvm_root)
            )
    except Exception as e:
        print("Conversion failed:", e)
