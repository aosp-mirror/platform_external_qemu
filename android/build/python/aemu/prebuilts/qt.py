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

import atexit
import glob
import logging
import os
import re
import shutil
import subprocess
import sys
from aemu.prebuilts.deps.common import EXE_SUFFIX
import aemu.prebuilts.deps.common as deps_common
import aemu.prebuilts.deps.mac as deps_mac
import aemu.prebuilts.deps.windows as deps_win
from pathlib import Path
import platform

AOSP_ROOT = Path(__file__).resolve().parents[7]
QT_VERSION = "6.5.3"
QT_SUBMODULES = [
    "qtbase",
    "qtdeclarative",
    "qtpositioning",
    "qtsvg",
    "qtimageformats",
    "qtwebchannel",
    "qtwebsockets",
    "qtwebengine"
]
AOSP_QT_SRC_PATH = os.path.join(AOSP_ROOT, "external", "qt5")
CMAKE_PATH = os.path.join(AOSP_ROOT, "prebuilts", "cmake", platform.system().lower() + "-x86", "bin")
CMAKE_EXE = os.path.join(CMAKE_PATH, "cmake" + EXE_SUFFIX)

# We must move the source code from external/qt5 to a shorter path because of path too long issue.
# Symlinking to a shorter path will not work either.
WIN_QT_TMP_LOCATION = os.path.join("C:/", "qttmp")
WIN_QT_SRC_SHORT_PATH = os.path.join(WIN_QT_TMP_LOCATION, "src")
WIN_QT_BUILD_PATH = os.path.join(WIN_QT_TMP_LOCATION, "bld")

def addToSearchPath(searchDir):
    os.environ["PATH"] = searchDir + os.pathsep + os.environ["PATH"]

def checkDependencies():
    # Dependencies for Qt 6 w/ QtWebEngine listed in https://wiki.qt.io/Building_Qt_6_from_Git
    logging.info("Checking for required build dependencies..")
    # - Python >= 3.6.x
    logging.info(">> Checking for Python >= 3.6.x (%s)", sys.version)
    deps_common.checkPythonVersion(min_vers=(3, 6))

    # Need perl to run qt5/init-repository
    logging.info(">> Checking for perl")
    deps_common.checkPerlVersion()

    # CMake >= 3.19 for Qt with QtWebEngine
    logging.info(">> Checking CMake >= 3.19")
    deps_common.checkCmakeVersion(min_vers=(3, 19))

    # Ninja >= 1.7.2 for Qt with QtWebEngine
    logging.info(">> Checking Ninja >= 1.7.2")
    deps_common.checkNinjaVersion(min_vers=(1, 7, 2))

    # Need node.js version 12 or later for QtWebEngine
    logging.info(">> Checking for node.js version >= 12")
    deps_common.checkNodeJsVersion(min_vers=(12, 0))

    # QtWebEngine needs python html5lib package
    logging.info(">> Checking pip.exe for html5lib")
    deps_common.checkPythonPackage("html5lib")

    # QtWebEngine requires GNUWin32 dependencies gperf, bison, flex
    # TODO(joshuaduong): Locally I installed bison from https://gnuwin32.sourceforge.net/packages.html, but maybe we use chocolatey on the buildbot.
    logging.info(">> Checking for gperf")
    deps_common.checkGperfVersion()
    logging.info(">> Checking for bison")
    deps_common.checkBisonVersion()
    logging.info(">> Checking for flex")
    deps_common.checkFlexVersion()

    if platform.system().lower() == "windows":
        # - Visual Studio 2019 v16.11+
        logging.info(">> Checking for Visual Studio 2019 v16.11+")
        deps_win.checkVSVersion(min_vers=(16, 11))
        # Windows 10 SDK >= 10.0.20348.0 (documentation says >= 10.0.19041, but configure complains
        # that it must be >= 10.0.20348.0).
        logging.info(">> Checking for Windows 10 SDK >= 10.0.20348.0")
        deps_win.checkWindowsSdk(min_vers=(10, 0, 20348, 0))
    elif platform.system().lower() == "darwin":
        # MacOS sdk >= 13
        deps_mac.checkMacOsSDKVersion(min_vers=(12, 0))

    return True

def flattenQtRepo(srcdir):
    """Checks out the given git submodules in the Qt repository.

    The Qt git repository is using git submodules, and relies on a script, `init-repository` to check out
    the git submodules.

    Args:
        srcdir (StrOrBytesPath): Path to the Qt source code.
    """
    perl_exe = "perl" + EXE_SUFFIX
    logging.info(">> CMD: %s init-repostory", perl_exe)
    try:
        # We need to set this git config so init-repository pulls down the git submodules correctly.
        # We might need to mirror these submodules as well.
        subprocess.run(args=["git", "config", "remote.origin.url", "https://github.com/qt/"],
                       cwd=srcdir, env=os.environ.copy())
        res = subprocess.run(args=[perl_exe, "init-repository", "-f"], cwd=srcdir, env=os.environ.copy())
        if res.returncode != 0:
            logging.critical("init-repository exited with non-zero code (%s)", res.returncode)
            exit(res.returncode)
        logging.debug(">> CMD: %s init-repostory succeeded", perl_exe)
    finally:
        subprocess.run(args=["git", "config", "--unset", "remote.origin.url"],
                       cwd=srcdir, env=os.environ.copy())

def configureQtBuild(srcdir, builddir, installdir):
    """Runs the configure.bat script for Qt.

    Args:
        srcdir (str): The location of the Qt source code.
        builddir (str): The location of the Qt build directory. Must be outside of the `srcdir`.
        This directory will be created in this function.
        installdir (str): The location of the Qt installation directory. Must be outside of the `srcdir`.
        This directory will be created in this function.
    """
    if os.path.exists(builddir):
        shutil.rmtree(builddir)
    os.makedirs(builddir)
    config_script = os.path.join(srcdir,
            ("configure.bat" if platform.system().lower() == "windows" else "configure"))
    conf_args = [config_script,
                 "-opensource",
                 "-confirm-license",
                 "-release",
                 "-force-debug-info",
                 "-shared",
                 "-no-rpath",
                 "-nomake", "examples",
                 "-nomake", "tests",
                 "-nomake", "tools",
                 "-skip", "qtdoc",
                 "-skip", "qttranslations",
                 "-skip", "qttools",
                 "-no-webengine-pepper-plugins",
                 "-no-webengine-printing-and-pdf",
                 "-no-webengine-webrtc",
                 "-no-webengine-spellchecker",
                 "-no-strip",
                 "-no-framework",
                 "-qtlibinfix", "AndroidEmu",
                 "-prefix", installdir]
    logging.info("[%s] Running %s", builddir, config_script)
    logging.info(conf_args)
    res = subprocess.run(args=conf_args, cwd=builddir, env=os.environ.copy())
    if res.returncode != 0:
        logging.critical("%s exited with non-zero code (%s)", config_script, res.returncode)
        exit(res.returncode)
    logging.info("%s succeeded", config_script)


def buildQt(submodules, builddir):
    """Builds the specified Qt submodules.

    Args:
        submodules (tuple(str)): The Qt submodules to build.
        builddir (str): The location of the Qt build directory.
    """
    for mod in submodules:
        cmake_build_cmd = ["cmake" + EXE_SUFFIX, "--build", ".", "--target", mod, "--parallel"]
        logging.info(cmake_build_cmd)
        res = subprocess.run(args=cmake_build_cmd, cwd=builddir, env=os.environ.copy())
        if res.returncode != 0:
            logging.critical("[%s] failed (%s)", cmake_build_cmd, res.returncode)
            exit(res.returncode)
    logging.info("Build succeeded")

def installQt(submodules, builddir, installdir):
    """Installs the specified Qt submodules to `installdir`.

    Args:
        submodules (tuple(str)): The Qt submodules to install.
        builddir (str): The location of the Qt build directory.
        installdir (str): The location of the Qt install directory.
    """
    logging.info("Installing Qt to %s", installdir)
    for mod in submodules:
        cmake_install_cmd = ["cmake" + EXE_SUFFIX, "--install", mod, "--prefix", installdir]
        logging.info(cmake_install_cmd)
        res = subprocess.run(args=cmake_install_cmd, cwd=builddir, env=os.environ.copy())
        if res.returncode != 0:
            logging.critical("[%s] failed (%s)", cmake_install_cmd, res.returncode)
            exit(res.returncode)
    logging.info("Installation succeeded")

def cleanup():
    if os.path.exists(WIN_QT_TMP_LOCATION):
        shutil.rmtree(WIN_QT_TMP_LOCATION)

def postInstall(installdir):
    # Create include.system/QtCore/qconfig.h from include/QtCore/qconfig.h
    src_qconfig = Path(installdir) / "include" / "QtCore" / "qconfig.h"
    dst_qconfig = Path(installdir) / "include.system" / "QtCore" / "qconfig.h"
    logging.info("Copy %s => %s", src_qconfig, dst_qconfig)
    os.makedirs(dst_qconfig.parent, exist_ok=True)
    shutil.copyfile(src_qconfig, dst_qconfig)

    if platform.system().lower() == "darwin":
        mac_postInstall(installdir)

def mac_postInstall(installdir):
    """Post-install steps specific for mac.
    """
    def changeToQt6Rpaths(file):
        libs = subprocess.check_output(["otool", "-LX", file], cwd=installdir,
                                        env=os.environ.copy(), encoding="utf-8").split("\n")
        qtlib_regex = ".*(libQt6.*AndroidEmu)\.6\.dylib"
        for lib in libs:
            s = re.search(qtlib_regex, lib)
            if s:
                old_lib = s.group(0).strip()
                new_lib = f"@rpath/{s.group(1)}.{QT_VERSION}.dylib"
                logging.info("%s: Changing %s => %s", os.path.basename(file), old_lib, new_lib)
                subprocess.check_call(
                    ["install_name_tool", "-change", f"{old_lib}", f"{new_lib}", file],
                    cwd=installdir, env=os.environ.copy())

    # Fix the rpaths. We want all libQt6 dependencies to point to @rpath/libQt6*.6.5.3.dylib instead
    # of @rpath/libQt6*.6.dylib. This removes the need to set DYLD_LIBRARY_PATH properly.
    for file in glob.iglob(os.path.join(installdir, "**", "*.dylib"), recursive=True):
        if os.path.islink(file):
            logging.info(">> Skipping symlink %s", file)
            continue
        else:
            logging.info(">> Fixing rpath for %s", file)
            basename = os.path.basename(file)
            logging.info("%s: Changing install name to @rpath/%s", basename, basename)
            subprocess.check_call(["install_name_tool", "-id", f"@rpath/{basename}", file],
                                  cwd=installdir, env=os.environ.copy())
            changeToQt6Rpaths(file)


    # Also need to fix the rpaths in the Qt executables we use.
    fix_exes = [
        f"{installdir}/libexec/moc",
        f"{installdir}/libexec/rcc",
        f"{installdir}/libexec/uic",
        f"{installdir}/libexec/QtWebEngineProcess"]
    for exe in fix_exes:
        changeToQt6Rpaths(exe)

def buildPrebuilt(args, prebuilts_out_dir):
    atexit.register(cleanup)

    qt_install_dir = os.path.join(prebuilts_out_dir, "qt")

    # Use cmake from our prebuilts
    addToSearchPath(CMAKE_PATH)
    logging.info(os.environ)

    if not checkDependencies():
        logging.fatal("Build environment does not have the required dependencies to build. Exiting..")
        exit(-1)

    # Qt source code is in external/qt5
    if not os.path.isdir(AOSP_QT_SRC_PATH):
        logging.fatal("%s does not exist", AOSP_QT_SRC_PATH)
        exit(-1)
    logging.info("QT source: %s", AOSP_QT_SRC_PATH)

    # TODO(joshuaduong): If we decide to flatten the repository at check-in, then this step won't be necessary.
    flattenQtRepo(AOSP_QT_SRC_PATH)

    if platform.system().lower() == "windows":
        # On Windows, We must make sure Qt source code is in a very short directory, otherwise we
        # may get a compiler error because of long paths issue.
        if os.path.exists(WIN_QT_TMP_LOCATION):
            os.remove(WIN_QT_TMP_LOCATION)
        logging.info("Creating Qt temp directory [%s]", WIN_QT_TMP_LOCATION)
        os.makedirs(WIN_QT_TMP_LOCATION)
        logging.info("Moving source directory to shorter path [%s ==> %s]",
            AOSP_QT_SRC_PATH, WIN_QT_SRC_SHORT_PATH)
        os.rename(AOSP_QT_SRC_PATH, WIN_QT_SRC_SHORT_PATH)
        qt_src_path = WIN_QT_SRC_SHORT_PATH
        qt_build_path = WIN_QT_BUILD_PATH
    else:
        qt_src_path = AOSP_QT_SRC_PATH
        qt_build_path = os.path.join(args.out, "build-qt")

    configureQtBuild(qt_src_path, qt_build_path, qt_install_dir)

    buildQt(QT_SUBMODULES, qt_build_path)

    installQt(QT_SUBMODULES, qt_build_path, qt_install_dir)

    postInstall(qt_install_dir)
    logging.info("Successfully built Qt!")
