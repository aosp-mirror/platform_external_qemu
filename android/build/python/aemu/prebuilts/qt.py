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
import aemu.prebuilts.deps.linux as deps_linux
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
    elif platform.system().lower() == "linux":
        pkgconfig_libs = [
            ("xrender", [0, 9, 0]),
            ("xcb-render", [1, 11]),
            ("xcb-renderutil", [0, 3, 9]),
            ("xcb-shape", [1, 11]),
            ("xcb-randr", [1, 11]),
            ("xcb-xfixes", [1, 11]),
            ("xcb-xkb", [1, 11]),
            ("xcb-sync", [1, 11]),
            ("xcb-shm", [1, 11]),
            ("xcb-icccm", [0, 3, 9]),
            ("xcb-keysyms", [0, 3, 9]),
            ("xcb-image", [0, 3, 9]),
            ("xcb-util", [0, 3, 9]),
            ("xcb-cursor", [0, 1, 1]),
            ("xkbcommon", [0, 5, 0]),
            ("xkbcommon-x11", [0, 5, 0]),
            ("fontconfig", [2, 6]),
            ("freetype2", [2, 3, 0]),
            ("xext", None),
            ("x11", None),
            ("xcb", [1, 11]),
            ("x11-xcb", [1, 3, 2]),
            ("sm", None),
            ("ice", None),
            ("glib-2.0", [2, 8, 3]),
            ("dbus-1", None),
            ("xcomposite", None),
            ("xcursor", None),
            ("xrandr", None),
            ("xshmfence", None),
            ("libcupsfilters", None),
        ]
        for libname, vers in pkgconfig_libs:
            if vers:
                logging.info("Checking pkg-config for (%s >= %s)", libname, '.'.join(map(str, vers)))
            else:
                logging.info("Checking pkg-config for %s", libname)
            deps_linux.checkPkgConfigLibraryExists(libname, vers)

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
                 "-submodules",
                 ",".join(QT_SUBMODULES),
                 "-nomake", "examples",
                 "-nomake", "tests",
                 "-nomake", "tools",
                 "-skip", "qtdoc",
                 "-skip", "qttranslations",
                 "-skip", "qttools",
                 "-no-feature-pdf",
                 "-no-feature-webengine-pepper-plugins",
                 "-no-feature-webengine-printing-and-pdf",
                 "-no-feature-webengine-webrtc",
                 "-no-feature-webengine-spellchecker",
                 "-no-feature-cups",
                 "-no-strip",
                 "-no-framework",
                 "-qtlibinfix", "AndroidEmu",
                 "-prefix", installdir]

    if platform.system().lower() == "linux":
        extra_conf_args = ["-linker", "lld",
                           "-platform", "linux-clang-libc++"]
        os.environ["CC"] = "clang"
        os.environ["CXX"] = "clang++"
        extra_cflags = "-m64 -fuse-ld=lld -fPIC"
        extra_cxxflags = "-stdlib=libc++ -m64 -fuse-ld=lld -fPIC"
        # Use libc++ instead of libstdc++
        conf_cmake_args = [ f"-DCMAKE_CXX_FLAGS={extra_cxxflags}", f"-DMAKE_C_FLAGS={extra_cflags}" ]
        # The QtWebEngine chromium build doesn't seem to pick up CXXFLAGS
        # envrionment variables. So let's just create some clang wrappers
        # to force use our flags.
        os.environ["CFLAGS"] = extra_cflags
        os.environ["CXXFLAGS"] = extra_cxxflags
        toolchain_dir = Path(builddir) / "toolchain"
        os.makedirs(Path(builddir) / "toolchain")

        with open(toolchain_dir / "clang", 'x') as f:
            f.write(f"#!/bin/sh\n/usr/bin/clang {extra_cflags} $@")
        os.chmod(toolchain_dir / "clang", 0o777)

        with open(toolchain_dir / "clang++", 'x') as f:
            f.write(f"#!/bin/sh\n/usr/bin/clang++ {extra_cxxflags} $@")
        os.chmod(toolchain_dir / "clang++", 0o777)

        addToSearchPath(str(toolchain_dir))

    if conf_cmake_args:
        conf_args.append("--")
        conf_args += conf_cmake_args

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
    cmake_build_cmd = ["cmake" + EXE_SUFFIX, "--build", "."]
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
    cmake_install_cmd = ["cmake" + EXE_SUFFIX, "--install", ".", "--prefix", installdir]
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
    elif platform.system().lower() == "linux":
        linux_postInstall(installdir)

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

def linux_postInstall(installdir):
    # Install prebuilt clang's libc++.so.1 to lib/ as Qt's compiler
    # tools (moc, rcc, uic) depends on it.
    src_libc = deps_common.getClangDirectory() / "lib" / "libc++.so.1"
    dst_libc = f"{installdir}/lib/libc++.so.1"
    logging.info(f"Copy {src_libc} ==> {dst_libc}")
    shutil.copyfile(src_libc, dst_libc)

    fix_exes = [
        f"{installdir}/libexec/moc",
        f"{installdir}/libexec/rcc",
        f"{installdir}/libexec/uic",
        f"{installdir}/libexec/QtWebEngineProcess"]
    # Add rpath to $ORIGIN/../lib so these executables can find libc++.so.1.
    # You can verify the rpath with `objdump -x <exe> | grep PATH`.
    # Also can check if libc++.so.1 is resolvable with `ldd <exe>`.
    for exe in fix_exes:
        logging.info(f"Adding rpath $ORIGIN/../lib to {exe}")
        subprocess.check_call(
            ["patchelf", "--set-rpath", "$ORIGIN/../lib", exe],
            cwd=installdir, env=os.environ.copy())

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
