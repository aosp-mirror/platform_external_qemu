#!/usr/bin/env python
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

import logging
import os
import shutil
import subprocess
import aemu.prebuilts.deps.common as deps_common
from pathlib import Path
import platform

AOSP_ROOT = Path(__file__).resolve().parents[7]
HOST_OS = platform.system().lower()
HOST_ARCH = platform.machine().lower()

AOSP_MOLTENVK_SRC_PATH = AOSP_ROOT / "external" / "moltenvk"
MOLTENVK_OUT_FILES = ["MoltenVK_icd.json", "libMoltenVK.dylib"]
CMAKE_PATH = os.path.join(AOSP_ROOT, "prebuilts", "cmake", HOST_OS + "-x86", "bin")
NINJA_PATH = os.path.join(AOSP_ROOT, "prebuilts", "ninja", HOST_OS + "-x86")

def fetchMoltenVkDependencies():
    logging.debug(">> CMD: ./fetchDependencies --macos")
    res = subprocess.run(args=["./fetchDependencies", "--macos"], cwd=AOSP_MOLTENVK_SRC_PATH,
                         env=os.environ.copy())
    if res.returncode != 0:
        logging.critical("fetchDependencies exited with non-zero code (%s)", res.returncode)
        exit(res.returncode)
    logging.debug(">> CMD: ./fetchDependencies --macos succeeded")

def buildMoltenVk():
    """Builds MoltenVK.

    Returns:
        Path: The directory of the build output, or None if build failed.
    """
    BUILD_DIR = AOSP_MOLTENVK_SRC_PATH / "Package" / "Release" / "MoltenVK" / "dylib" / "macOS"
    build_cmd = ["make", "macos"]
    logging.info(build_cmd)
    # Just build it in the source tree
    res = subprocess.run(args=build_cmd, cwd=AOSP_MOLTENVK_SRC_PATH, env=os.environ.copy())
    if res.returncode != 0:
        logging.critical("[%s] failed (%s)", build_cmd, res.returncode)
        exit(res.returncode)

    for f in MOLTENVK_OUT_FILES:
        if not os.path.exists(BUILD_DIR / f):
            logging.critical("Build output missing %s", str(BUILD_DIR / f))
            return None

    logging.info("Build succeeded")
    return BUILD_DIR

def installMoltenVk(builddir, installdir):
    """Installs the output files from `builddir` to `installdir`.

    Args:
        builddir (str): The location of the moltenvk build directory.
        installdir (str): The location of the moltenvk install directory.
    """
    logging.info("Installing MoltenVk to %s", installdir)
    os.makedirs(installdir)

    for f in MOLTENVK_OUT_FILES:
        src_file = builddir / f
        dst_file = installdir / f
        logging.info("Copy %s => %s", str(src_file), str(dst_file))
        shutil.copyfile(src_file, dst_file)
    logging.info("Installed MoltenVK files to %s", installdir)

def buildPrebuilt(args, prebuilts_out_dir):
    # Use cmake from our prebuilts
    deps_common.addToSearchPath(CMAKE_PATH)
    # Use ninja from our prebuilts
    deps_common.addToSearchPath(NINJA_PATH)
    logging.info(os.environ)

    # moltenvk source code is in external/moltenvk
    if not os.path.isdir(AOSP_MOLTENVK_SRC_PATH):
        logging.fatal("%s does not exist", AOSP_MOLTENVK_SRC_PATH)
        exit(-1)
    logging.info("MoltenVk source: %s", AOSP_MOLTENVK_SRC_PATH)

    fetchMoltenVkDependencies()

    build_dir = buildMoltenVk()
    if not build_dir:
        logging.critical("MoltenVK build failed")
        exit(1)

    moltenvk_install_dir = Path(prebuilts_out_dir) / "moltenvk"
    installMoltenVk(build_dir, moltenvk_install_dir)
    logging.info("Successfully built MoltenVk!")