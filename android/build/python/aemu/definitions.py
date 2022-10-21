#!/usr/bin/env python
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
import json
import logging
import os
import platform
import re
import subprocess

EXE_POSTFIX = ""
if platform.system() == "Windows":
    EXE_POSTFIX = ".exe"

AOSP_ROOT = ""


CPU_ARCHITECTURE_MAP = {
    # Arm based
    "aarch64": "aarch64",
    "aarch64_be": "aarch64",
    "arm64": "aarch64",
    "armv8b": "aarch64",
    "armv8l": "aarch64",
    # Intel based.
    "amd64": "x86_64",
    "i686": "x86_64",
    "x86_64": "x86_64",
}


def find_aosp_root():
    path = os.path.abspath(os.getcwd())
    qemu = os.path.join("external", "qemu")
    if path.rfind(qemu) != -1:
        return path[: path.rfind(qemu)]

    return os.path.abspath(
        os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "..",
            "..",
            "..",
            "..",
            "..",
            "..",
        )
    )


def set_aosp_root(root):
    global AOSP_ROOT
    AOSP_ROOT = root


def get_aosp_root():
    return AOSP_ROOT


def get_qemu_root():
    return os.path.abspath(os.path.join(get_aosp_root(), "external", "qemu"))


def get_cmake_dir():
    """Return the cmake directory for the current platform."""
    return os.path.join(
        get_aosp_root(), "prebuilts/cmake/%s-x86/bin/" % platform.system().lower()
    )


def get_cmake():
    """Return the path of cmake executable."""
    return os.path.join(get_cmake_dir(), "cmake%s" % EXE_POSTFIX)


def get_ctest():
    """Return the path of ctest executable."""
    return os.path.join(get_cmake_dir(), "ctest%s" % EXE_POSTFIX)


def get_visual_studio():
    """Gets the path to visual studio, or None if not found."""
    try:
        prgrfiles = os.getenv("ProgramFiles(x86)", "C:\Program Files (x86)")
        res = subprocess.check_output(
            [
                os.path.join(
                    prgrfiles, "Microsoft Visual Studio", "Installer", "vswhere.exe"
                ),
                "-requires",
                "Microsoft.VisualStudio.Workload.NativeDesktop",
                "-format",
                "json",
                "-utf8",
            ]
        )
        vsresult = json.loads(res)
        if len(vsresult) == 0:
            raise ValueError("No visual studio with the native desktop load available.")

        for install in vsresult:
            logging.info("Considering %s", install["displayName"])
            candidates = list(
                recursive_iglob(
                    install["installationPath"], [lambda f: f.endswith("vcvars64.bat")]
                )
            )
            if len(candidates) > 0:
                logging.info('Accepted, setting env with "%s"', candidates[0])
                return candidates[0]
    except:
        logging.error(
            "Error while trying to detect visual studio install", exc_info=True
        )
    logging.error(
        "Unable to detect a visual studio installation with the native desktop workload."
    )
    return None


# Functions that determine if a file is executable.
is_executable = {
    "Windows": [lambda f: f.endswith(".exe") or f.endswith(".dll")],
    "Linux": [lambda f: os.access(f, os.X_OK), lambda f: f.endswith(".so")],
    "Darwin": [lambda f: os.access(f, os.X_OK), lambda f: f.endswith(".dylib")],
}


def recursive_iglob(rootdir, pattern_fns):
    """Recursively glob the rootdir for any file that matches the pattern functions."""
    for root, _, filenames in os.walk(rootdir):
        for filename in filenames:
            fname = os.path.join(root, filename)
            for pattern_fn in pattern_fns:
                if pattern_fn(fname):
                    yield fname


def read_simple_properties(fname):
    res = {}
    with open(fname, "r") as props:
        for line in props.readlines():
            k, v = line.split("=")
            res[k.strip()] = v.strip()
    return res


def is_crosscompile(target):
    """True if the given target needs to be cross compiled.

    Args:
        target (str): The complete target.

    Raises:
        Exception: If the given target cannot be inferred or is not supported.


    Returns:
        bool: True if we will crosscompile on this machine for the given target.
    """
    target = target.lower().strip()
    match = re.match("(windows|linux|darwin)([-_](aarch64|x86_64))?", target)
    if not (match and match.group(3)):
        raise Exception("Malformed target: {}.".format(target))
    aarch = CPU_ARCHITECTURE_MAP.get(platform.machine().lower(), "unknown_cpu")
    if aarch == "arm64":
        aarch = "aarch64"
    else:
        # Ok maybe python is running under rosetta, if so the uname.version
        # will have have something along RELEASE_ARM64_T6000 in it
        uname = platform.uname()
        if "ARM64" in uname.version and uname.system == "Darwin":
            aarch = "aarch64"

    return aarch != match.group(3)


def infer_target(target):
    """Infers the full target name given the potential partial target name.

       A complete target name looks like: ${OS}_{$AARCH} where
       OS = [windows, linux, darwin] and AARCH = [aarch64, x86_64]


    Args:
        target (str): The desired target you wish to compile for

    Raises:
        Exception: If the given target cannot be inferred or is not supported.

    Returns:
        str: The actual complete target name.

    """
    target = target.lower().strip()
    host = platform.system().lower()
    match = re.match("(windows|linux|darwin)([-_](aarch64|x86_64))?", target)

    if match:
        if not match.group(3):
            # Let's infer the architecture
            aarch = platform.machine()

            # Note, this is far from complete, and is best effort, if this does
            # not work you should specify the architecture yourself as the
            # warning is pointing out.
            if aarch == "arm64":
                aarch = "aarch64"
            if aarch != "aarch64":
                aarch = "x86_64"

            if host != target:
                logging.warning(
                    "\n\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="
                )
                logging.warning(
                    "--- WARNNG ---  You are cross compiling on %s with target %s and did not specify a machine architecture. Assuming: %s",
                    host,
                    target,
                    aarch,
                )
                logging.warning(
                    "--- WARNNG ---  If this does not work you should really specify the desired machine architecture"
                )
                logging.warning(
                    "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n"
                )

            target = target + "-" + aarch
        else:
            target = match.group(1) + "-" + match.group(3)

    if target not in ENUMS["Toolchain"]:
        raise Exception(
            "Target {} does not exist, we only support: {}.".format(
                target, ", ".join(list(ENUMS["Toolchain"].keys()))
            )
        )

    return target


ENUMS = {
    "Crash": {
        "prod": [
            "-DOPTION_CRASHUPLOAD=PROD",
            "-DBREAKPAD_API_URL=https://prod-crashsymbolcollector-pa.googleapis.com",
        ],
        "staging": [
            "-DOPTION_CRASHUPLOAD=STAGING",
            "-DBREAKPAD_API_URL=https://staging-crashsymbolcollector-pa.googleapis.com",
        ],
        "none": ["-DOPTION_CRASHUPLOAD=NONE"],
    },
    "BuildConfig": {
        "debug": ["-DCMAKE_BUILD_TYPE=Debug"],
        "release": ["-DCMAKE_BUILD_TYPE=Release"],
    },
    "Make": {"config": "configure only", "check": "check"},
    "Generator": {
        "xcode": ["-G", "Xcode"],
        "ninja": ["-G", "Ninja"],
        "make": ["-G", "Unix Makefiles"],
        "visualstudio": ["-G", "Visual Studio 15 2017 Win64"],
    },
    "Toolchain": {
        "windows-x86_64": "toolchain-windows_msvc-x86_64.cmake",
        "linux-x86_64": "toolchain-linux-x86_64.cmake",
        "darwin-x86_64": "toolchain-darwin-x86_64.cmake",
        "linux-aarch64": "toolchain-linux-aarch64.cmake",
        "darwin-aarch64": "toolchain-darwin-aarch64.cmake",
    },
}
