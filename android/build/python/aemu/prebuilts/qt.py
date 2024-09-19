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
HOST_OS = platform.system().lower()
HOST_ARCH = platform.machine().lower()

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
QT_NOWEBENGINE_SUBMODULES = [
    "qtbase",
    "qtsvg",
    "qtimageformats",
]
AOSP_QT_SRC_PATH = os.path.join(AOSP_ROOT, "external", "qt5")
CMAKE_PATH = os.path.join(AOSP_ROOT, "prebuilts", "cmake", HOST_OS + "-x86", "bin")
NINJA_PATH = os.path.join(AOSP_ROOT, "prebuilts", "ninja", HOST_OS + "-x86")

# We must move the source code from external/qt5 to a shorter path because of path too long issue.
# Symlinking to a shorter path will not work either.
WIN_QT_TMP_LOCATION = os.path.join("C:\\", "qttmp")
WIN_QT_SRC_SHORT_PATH = os.path.join(WIN_QT_TMP_LOCATION, "src")
WIN_QT_BUILD_PATH = os.path.join(WIN_QT_TMP_LOCATION, "bld")

# Qt6 python scripts do not work with python 3.12, so let's hardcode the 3.11 version in our
# buildbots.
MAC_ARM64_PYTHON_3_11 = os.path.join("/opt", "homebrew", "Cellar", "python@3.11", "3.11.9_1", "libexec", "bin")
MAC_X64_PYTHON_3_11 = os.path.join("/usr", "local", "Cellar", "python@3.11", "3.11.9_1", "libexec", "bin")

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

    if HOST_OS == "linux" and HOST_ARCH == "aarch64":
        logging.info("Skipping QtWebEngine dependency check for linux-aarch64")
    else:
        # Need node.js version 12 or later for QtWebEngine
        logging.info(">> Checking for node.js version >= 12")
        deps_common.checkNodeJsVersion(min_vers=(12, 0))

        # If we ever switch back to AOSP python, we need to uncomment out the stuff below.
        """
        # QtWebEngine needs python html5lib package
        #
        # Since we use a custom python installation, we need to manually install these packages and
        # provide the location to these packages.
        PYTHON_INDEX_URL = os.path.join(AOSP_ROOT, "external", "adt-infra", "devpi", "repo", "simple")
        INDEX_FILE_PREFIX = "file:///" if HOST_OS == "windows" else "file://"
        pip.main(["install", "html5lib", '-i', f"{INDEX_FILE_PREFIX}{PYTHON_INDEX_URL}"])
        """
        logging.info(">> Checking for python package html5lib")
        deps_common.checkPythonPackage("html5lib")

        # QtWebEngine requires GNUWin32 dependencies gperf, bison, flex
        # TODO(joshuaduong): Locally I installed bison from https://gnuwin32.sourceforge.net/packages.html, but maybe we use chocolatey on the buildbot.
        logging.info(">> Checking for gperf")
        deps_common.checkGperfVersion()
        logging.info(">> Checking for bison")
        deps_common.checkBisonVersion()
        logging.info(">> Checking for flex")
        deps_common.checkFlexVersion()

    if HOST_OS == "windows":
        # - Visual Studio 2019 v16.11+
        logging.info(">> Checking for Visual Studio 2019 v16.11+")
        deps_win.checkVSVersion(min_vers=(16, 11))
        # Windows 10 SDK >= 10.0.20348.0 (documentation says >= 10.0.19041, but configure complains
        # that it must be >= 10.0.20348.0).
        logging.info(">> Checking for Windows 10 SDK >= 10.0.20348.0")
        deps_win.checkWindowsSdk(min_vers=(10, 0, 20348, 0))
    elif HOST_OS == "darwin":
        # MacOS sdk >= 13
        deps_mac.checkMacOsSDKVersion(min_vers=(12, 0))
    elif HOST_OS == "linux" and HOST_ARCH != "aarch64":
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
        # patchelf required to modify rpaths during installation
        if not deps_common.checkExeIsOnPath("patchelf"):
            return False
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

def configureQtBuild(srcdir, builddir, installdir, qtsubmodules, crosscompile_target=None,
                     qt_host_path=None):
    """Runs the configure.bat script for Qt.

    Args:
        srcdir (str): The location of the Qt source code.
        builddir (str): The location of the Qt build directory. Must be outside of the `srcdir`.
        This directory will be created in this function.
        installdir (str): The location of the Qt installation directory. Must be outside of the `srcdir`.
        This directory will be created in this function.
        qtsubmodules (list(str)): The list of Qt submodules to build.
        crosscompile_target (str): The target to cross-compile to. Defaults to None. This currently
        only works for targeting linux_aarch64 target on linux_x86_64 host.
        qt_host_path (str): The path to where Qt was installed on the host machine. This parameter
        is only used in cross compilation builds.
    """
    if os.path.exists(builddir):
        shutil.rmtree(builddir)
    old_umask = os.umask(0o027)
    os.makedirs(builddir)
    os.umask(old_umask)

    config_script = os.path.join(srcdir,
            ("configure.bat" if HOST_OS == "windows" else "configure"))
    conf_args = [config_script,
                 "-opensource",
                 "-confirm-license",
                 "-release",
                 "-force-debug-info",
                 "-shared",
                 "-no-rpath",
                 "-submodules",
                 ",".join(qtsubmodules),
                 "-nomake", "examples",
                 "-nomake", "tests",
                 "-nomake", "tools",
                 "-skip", "qtdoc",
                 "-skip", "qttranslations",
                 "-skip", "qttools",
                 "-no-feature-pdf",
                 "-no-feature-printdialog",
                 "-no-feature-printsupport",
                 "-no-feature-printer",
                 "-no-feature-printpreviewdialog",
                 "-no-feature-printpreviewwidget",
                 "-no-feature-webengine-pepper-plugins",
                 "-no-feature-webengine-printing-and-pdf",
                 "-no-feature-webengine-webrtc",
                 "-no-feature-webengine-spellchecker",
                 "-no-feature-cups",
                 "-no-strip",
                 "-no-framework",
                 "-qtlibinfix", "AndroidEmu",
                 "-prefix", installdir]

    if HOST_OS != "windows":
        # qtwebengine build fails without opengl.
        conf_args += ["-no-opengl"]

    if HOST_OS == "windows":
        conf_args += ["-platform", "win32-msvc"]
    elif HOST_OS == "linux":
        if HOST_ARCH == "aarch64" or crosscompile_target == "linux_aarch64":
            # Cross-compiling requires a host installation of Qt, so we must build Qt for
            # linux-x86_64 prior to cross-compiling to linux_aarch64.
            conf_args += ["-platform", "linux-aarch64-gnu-g++"]
            if crosscompile_target:
                if not qt_host_path:
                    logging.fatal(f"No Qt host path was provided for {crosscompile_target} cross-compilation.")
                # We have to disable a lot of things for cross-compilation without a full sysroot.
                logging.warning("GUI support is not supported in cross-compilation.")
                conf_args += [
                          "-device-option", "CROSS_COMPILE=aarch64-linux-gnu-",
                          "-no-glib", "-qt-host-path", qt_host_path, "-qt-pcre", "-qt-zlib",
                          "-qt-doubleconversion", "-qt-freetype", "-qt-harfbuzz",
                          "-no-feature-gssapi", "-no-feature-brotli", "-no-xcb", "-no-xkbcommon",
                          "-qt-webp", "-no-libudev", "-no-mtdev", "-no-linuxfb"]
                conf_args += ["--", "-DCMAKE_SYSTEM_NAME=Linux", "-DCMAKE_SYSTEM_PROCESSOR=arm",
                            f"-DQT_HOST_PATH={qt_host_path}", "-DCMAKE_LINKER_FLAGS=-Wl,--as-needed"]
                os.environ["CC"] = "aarch64-linux-gnu-gcc"
                os.environ["CXX"] = "aarch64-linux-gnu-g++"
            extra_cflags = "-march=armv8-a"
            extra_cxxflags = "-march=armv8-a"
            os.environ["CFLAGS"] = extra_cflags
            os.environ["CXXFLAGS"] = extra_cxxflags
        else:
            conf_args += ["-linker", "lld",
                          "-platform", "linux-clang-libc++",
                          "-no-glib"]

            os.environ["CC"] = "clang"
            os.environ["CXX"] = "clang++"
            extra_cflags = "-m64 -fuse-ld=lld -fPIC"
            extra_cxxflags = "-stdlib=libc++ -m64 -fuse-ld=lld -fPIC"
            # Use libc++ instead of libstdc++
            conf_cmake_args = [ f"-DCMAKE_CXX_FLAGS={extra_cxxflags}", f"-DMAKE_C_FLAGS={extra_cflags}" ]
            conf_args.append("--")
            conf_args += conf_cmake_args

            # The QtWebEngine chromium build doesn't seem to pick up CXXFLAGS
            # envrionment variables. So let's just create some clang wrappers
            # to force use our flags.
            os.environ["CFLAGS"] = extra_cflags
            os.environ["CXXFLAGS"] = extra_cxxflags
            toolchain_dir = Path(builddir) / "toolchain"
            os.makedirs(Path(builddir) / "toolchain")

            if HOST_ARCH == "aarch64":
                clang_dir = "/usr"
            else:
                clang_dir = deps_common.getClangDirectory()

            with open(toolchain_dir / "clang", 'x') as f:
                f.write(f"#!/bin/sh\n{clang_dir}/bin/clang {extra_cflags} $@")
            os.chmod(toolchain_dir / "clang", 0o777)

            with open(toolchain_dir / "clang++", 'x') as f:
                f.write(f"#!/bin/sh\n{clang_dir}/bin/clang++ {extra_cxxflags} $@")
            os.chmod(toolchain_dir / "clang++", 0o777)

            with open(toolchain_dir / "ar", 'x') as f:
                f.write(f"#!/bin/sh\n{clang_dir}/bin/llvm-ar $@")
            os.chmod(toolchain_dir / "ar", 0o777)

            with open(toolchain_dir / "lld", 'x') as f:
                f.write(f"#!/bin/sh\n{clang_dir}/bin/lld $@")
            os.chmod(toolchain_dir / "lld", 0o777)

            os.environ["LD_LIBRARY_PATH"] = str(deps_common.getClangDirectory() / "lib")

            deps_common.addToSearchPath(str(toolchain_dir))

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

def cleanupQtTmpDirectory():
    if HOST_OS == "windows" and os.path.exists(WIN_QT_TMP_LOCATION):
        # os.remove and shutils.rmtree may fail if any file in the tmp directory is read-only.
        # So let's just use rmdir instead to destroy it.
        subprocess.run(["rmdir", "/S", "/Q", WIN_QT_TMP_LOCATION], shell=True)

def cleanup():
    cleanupQtTmpDirectory()

def postInstall(installdir, target, is_webengine):
    # Create include.system/QtCore/qconfig.h from include/QtCore/qconfig.h
    src_qconfig = Path(installdir) / "include" / "QtCore" / "qconfig.h"
    dst_qconfig = Path(installdir) / "include.system" / "QtCore" / "qconfig.h"
    logging.info("Copy %s => %s", src_qconfig, dst_qconfig)
    os.makedirs(dst_qconfig.parent, exist_ok=True)
    shutil.copyfile(src_qconfig, dst_qconfig)

    if HOST_OS == "darwin":
        mac_postInstall(installdir)
    elif HOST_OS == "linux":
        linux_postInstall(installdir, target, is_webengine)

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
        if not os.path.isfile(exe):
            logging.info(f"Skipping rpath fix for file {exe}")
            continue
        changeToQt6Rpaths(exe)
        # Add the search path to the executables so we can use them without setting environment
        # variables.
        res = subprocess.run(args=["install_name_tool", "-add_rpath", "@loader_path/../lib", exe],
                              cwd=installdir, env=os.environ.copy(), encoding="utf-8")
        if res.returncode != 0:
            logging.critical("install_name_tool -add_rpath failed (%d)", res.returncode)
            exit(res.returncode)

def linux_postInstall(installdir, target, is_webengine):
    # Install prebuilt clang's libc++.so.1 and libc++abi.so.1 to lib/ as Qt's compiler
    # tools (moc, rcc, uic) depends on it.
    if HOST_ARCH == "aarch64":
        sysroot_dir = Path("/lib/aarch64-linux-gnu")
    else:
        clang_libs = ["libc++.so.1", "libc++abi.so.1"]
        clang_dir = deps_common.getClangDirectory()
        sysroot_dir = Path("/lib/x86_64-linux-gnu")
        for lib in clang_libs:
            src_lib = clang_dir / "lib" / lib
            dst_lib = f"{installdir}/lib/{lib}"
            logging.info(f"Copy {src_lib} ==> {dst_lib}")
            shutil.copyfile(src_lib, dst_lib)

    # We also need additional libraries from the sysroot for linux
    sysroot_libs = [
        "libpcre2-16.so.0",
        "libfreetype.so.6",
        "libxkbcommon.so.0",
        "libXau.so.6",
        "libXdmcp.so.6",
        "libX11-xcb.so.1",
        "libxcb-xkb.so.1",
        "libxcb-cursor.so.0",
        "libxcb-icccm.so.4",
        "libxcb-image.so.0",
        "libxcb-keysyms.so.1",
        "libxcb-randr.so.0",
        "libxcb-render-util.so.0",
        "libxcb-render.so.0",
        "libxcb-shape.so.0",
        "libxcb-shm.so.0",
        "libxcb-sync.so.1",
        "libxcb-util.so.1",
        "libxcb-xfixes.so.0",
        "libxkbcommon-x11.so.0",
        "libfontconfig.so.1"]
    if target == "linux" and is_webengine:
        sysroot_libs += ["libjpeg.so.8"]
    if target == "linux" and HOST_ARCH == "aarch64":
        # Need additional libraries to build emulator
        sysroot_libs += [
            "libxcb.so.1",
            "libX11.so.6",
            "libz.so.1",
            "libpng16.so.16",
            "libexpat.so.1",
            "libuuid.so.1",
            "libdbus-1.so.3",
            "libsystemd.so.0",
            "liblzma.so.5",
            "liblz4.so.1",
            "libgcrypt.so.20",
            "libgpg-error.so.0",
            "libbsd.so.0"]
    for lib in sysroot_libs:
        src_lib = sysroot_dir / lib
        dst_lib = f"{installdir}/lib/{lib}"
        logging.info(f"Copy {src_lib} ==> {dst_lib}")
        shutil.copyfile(src_lib, dst_lib)
    if target == "linux" and HOST_ARCH == "aarch64":
        # These symlinks are needed to make emulator build compile
        os.symlink("libX11.so.6", f"{installdir}/lib/libX11.so")
        os.symlink("libXau.so.6", f"{installdir}/lib/libXau.so")

    if target != "linux_aarch64" and HOST_ARCH != "aarch64":
        # need libunwind.so.1 for syncqt
        src_libunwind = AOSP_ROOT / "prebuilts" / "android-emulator-build" / "common" / "libunwind" \
            / "linux-x86_64" / "lib" / "libunwind.so"
        dst_libunwind = f"{installdir}/lib/libunwind.so.1"
        logging.info(f"Copy {src_libunwind} ==> {dst_libunwind}")
        shutil.copyfile(src_libunwind, dst_libunwind)

    # Need syncqt, qvkgen on linux-x86_64 for linux_aarch64 cross-compile build
    fix_exes = [
        f"{installdir}/libexec/moc",
        f"{installdir}/libexec/rcc",
        f"{installdir}/libexec/uic",
        f"{installdir}/libexec/syncqt",
        f"{installdir}/libexec/qvkgen",
        f"{installdir}/libexec/cmake_automoc_parser",
        f"{installdir}/libexec/QtWebEngineProcess"]
    # Add rpath to $ORIGIN/../lib so these executables can find libc++.so.1.
    # You can verify the rpath with `objdump -x <exe> | grep PATH`.
    # Also can check if libc++.so.1 is resolvable with `ldd <exe>`.
    for exe in fix_exes:
        if not os.path.isfile(exe):
            logging.info(f"Skipping rpath fix for file {exe}")
            continue
        logging.info(f"Adding rpath $ORIGIN/../lib to {exe}")
        # patchelf may use RUNPATH instead of RPATH. We can force RPATH by using --force-rpath.
        subprocess.check_call(
            ["patchelf", "--force-rpath", "--set-rpath", "$ORIGIN/../lib", exe],
            cwd=installdir, env=os.environ.copy())

def buildPrebuilt(args, prebuilts_out_dir):
    atexit.register(cleanup)

    if HOST_OS == "windows":
        VS_INSTALL_PATH = os.environ.get("VS2022_INSTALL_PATH")
        if VS_INSTALL_PATH:
            # The existence of the environment variable indicates we are on an old-style buildbot
            # that does not have the docker configuration.
            vcvarsall = os.path.join(VS_INSTALL_PATH, "VC", "Auxiliary", "Build", "vcvarsall.bat")
            if not os.path.exists(vcvarsall):
                logging.fatal(f"[{vcvarsall}] does not exist")
                exit(-1)
            deps_win.inheritSubprocessEnv([vcvarsall, "amd64", ">NUL", "2>&1"])

    if HOST_OS == "linux" and HOST_ARCH == "aarch64":
        logging.info("Using system-installed cmake/ninja on linux-aarch64 host")
    else:
        # Use cmake from our prebuilts
        deps_common.addToSearchPath(CMAKE_PATH)
        # Use ninja from our prebuilts
        deps_common.addToSearchPath(NINJA_PATH)
    logging.info(os.environ)

    if HOST_OS == "darwin":
        if HOST_ARCH == "aarch64":
            deps_common.addToSearchPath(MAC_ARM64_PYTHON_3_11)
        else:
            deps_common.addToSearchPath(MAC_X64_PYTHON_3_11)

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

    if HOST_OS == "windows":
        # On Windows, We must make sure Qt source code is in a very short directory, otherwise we
        # may get a compiler error because of long paths issue.
        cleanupQtTmpDirectory()
        logging.info("Creating Qt temp directory [%s]", WIN_QT_TMP_LOCATION)
        old_umask = os.umask(0o027)
        os.makedirs(WIN_QT_TMP_LOCATION)
        os.umask(old_umask)
        logging.info("Moving source directory to shorter path [%s ==> %s]",
            AOSP_QT_SRC_PATH, WIN_QT_SRC_SHORT_PATH)
        os.rename(AOSP_QT_SRC_PATH, WIN_QT_SRC_SHORT_PATH)
        qt_src_path = WIN_QT_SRC_SHORT_PATH
        qt_build_path = WIN_QT_BUILD_PATH
    else:
        qt_src_path = AOSP_QT_SRC_PATH
        qt_build_path = os.path.join(args.out, "build-qt")

    # Build the nowebengine variants as well for linux/mac
    if HOST_OS == "linux" or HOST_OS == "darwin":
        logging.info("Building Qt 6 (no QtWebEngine)")
        qt_install_dir = os.path.join(prebuilts_out_dir, "qt-nowebengine")
        configureQtBuild(qt_src_path, qt_build_path, qt_install_dir, QT_NOWEBENGINE_SUBMODULES)
        buildQt(QT_NOWEBENGINE_SUBMODULES, qt_build_path)
        installQt(QT_NOWEBENGINE_SUBMODULES, qt_build_path, qt_install_dir)
        postInstall(qt_install_dir, args.target, False)
        shutil.rmtree(qt_build_path)

    if HOST_OS == "linux" and HOST_ARCH == "aarch64":
        logging.info("Skipping QtWebEngine build on linux-aarch64 machine")
    else:
        logging.info("Building Qt6 w/ QtWebEngine")
        qt_install_dir = os.path.join(prebuilts_out_dir, "qt")
        configureQtBuild(qt_src_path, qt_build_path, qt_install_dir, QT_SUBMODULES)
        buildQt(QT_SUBMODULES, qt_build_path)
        installQt(QT_SUBMODULES, qt_build_path, qt_install_dir)
        postInstall(qt_install_dir, args.target, True)

    # Since linux_aarch64 cross-compilation requires having a host build of Qt, let's just build it
    # in the linux x86_64 host instead of creating an additional linux_aarch64 build target.
    if HOST_OS == "linux" and HOST_ARCH != "aarch64":
        logging.info("Cross-compiling Qt 6 (no QtWebEngine) for linux_aarch64")
        crosscompile_target = "linux_aarch64"
        qt_install_dir = os.path.join(prebuilts_out_dir, "qt-nowebengine-linux_aarch64")
        qt_host_path = os.path.join(prebuilts_out_dir, "qt-nowebengine")
        configureQtBuild(qt_src_path, qt_build_path, qt_install_dir, QT_NOWEBENGINE_SUBMODULES,
                         crosscompile_target, qt_host_path)
        buildQt(QT_NOWEBENGINE_SUBMODULES, qt_build_path)
        installQt(QT_NOWEBENGINE_SUBMODULES, qt_build_path, qt_install_dir)
        postInstall(qt_install_dir, "linux_aarch64", False)
        shutil.rmtree(qt_build_path)
    logging.info("Successfully built Qt!")
