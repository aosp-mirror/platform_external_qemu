#!/usr/bin/env python3
import subprocess
import argparse


def get_git_pkgversion(directory):
    try:
        git_desc = subprocess.check_output(
            ["git", "-C", directory, "describe", "--match", "v*", "--dirty"],
            stderr=subprocess.PIPE,
            text=True,
        )
        return git_desc.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def generate_version_macros(directory, pkgversion, version):
    if not pkgversion:
        git_pkgversion = get_git_pkgversion(directory)
        pkgversion = git_pkgversion if git_pkgversion else ""

    fullversion = f"{version} ({pkgversion})" if pkgversion else version

    return f"""\
#define QEMU_PKGVERSION "{pkgversion}"
#define QEMU_FULL_VERSION "{fullversion}"
"""


def main():
    parser = argparse.ArgumentParser(description="Generate QEMU version macros.")
    parser.add_argument("directory", help="Path to the directory")
    parser.add_argument("pkgversion", help="Package version")
    parser.add_argument("version", help="QEMU version")

    args = parser.parse_args()

    result = generate_version_macros(args.directory, args.pkgversion, args.version)
    print(result)


if __name__ == "__main__":
    main()
