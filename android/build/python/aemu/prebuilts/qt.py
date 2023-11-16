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
import logging
import os
import re
import subprocess
import sys

QT_SUBMODULES = ["qtbase", "qtsvg", "qtimageformats", "qtwebengine"]
QT_SRC_PATH = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "..", "..", "..", "..", "..", "..", "qt5")
# We must move the source code from external/qt5 to a shorter path because of path too long issue.
# Symlinking to a shorter path will not work either.
QT_TMP_LOCATION = os.path.join("C:/", "qttmp")
QT_SRC_SHORT_PATH = os.path.join(QT_TMP_LOCATION, "src")
QT_BUILD_PATH = os.path.join(QT_TMP_LOCATION, "bld")
CMAKE_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                          "..", "..", "..", "..", "..", "prebuilts", "cmake", "windows-x86", "bin")
CMAKE_EXE = os.path.join(CMAKE_PATH, "cmake.exe")

def checkVersion(vers_cmd, vers_regex, min_vers):
    """Checks the executable version against the version requirements.

    Args:
        vers_cmd (str): A shell command to get the version information.
        vers_regex (str): The regular expression to filter for the version in vers_cmd output.
        min_vers (tuple(int)): The minimum required version. For example, version 12.0.1 = (12, 0, 1).

    Returns:
        bool: True if the version requirements are met, False otherwise.
    """
    try:
        res = subprocess.check_output(args=vers_cmd, env=os.environ.copy()).decode().strip()
        vers_str = re.search(vers_regex, res)
        logging.info("{} returned [{}], version=[{}]".format(vers_cmd, res, vers_str.group(0)))
        vers = tuple(map(int, vers_str.group(0).split('.')))
        if vers < min_vers:
            logging.critical("{} returned [{}] is not at least version {}".format(vers_cmd, vers_str.group(0), min_vers))
            return False
    except subprocess.CalledProcessError:
        logging.critical("Encountered problem executing [{}]".format(vers_cmd))
        return False

    return True

def checkDependencies():
    # Dependencies for Qt 6 w/ QtWebEngine listed in https://wiki.qt.io/Building_Qt_6_from_Git
    logging.info("Checking for required build dependencies..")
    # - Python >= 3.6.x
    logging.info(">> Checking for Python >= 3.6.x ({})".format(sys.version))
    if sys.version_info < (3, 6):
        logging.critical("Python version {} does not meet minimum requirements".format(sys.version))
        return False
    logging.info("Found suitable python version ({})".format(sys.version))

    # - Visual Studio 2019 v16.11+
    logging.info(">> Checking for Visual Studio 2019 v16.11+")
    vscheck = os.path.join("C:/", "Program Files (x86)", "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.isfile(vscheck):
        logging.critical("{} file not found. Unable to check VS version".format(vscheck))
        return False
    if not checkVersion(vers_cmd="{} -latest -property installationVersion".format(vscheck),
                        vers_regex="[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*",
                        min_vers=(16, 11)):
        return False

    # Windows 10 SDK >= 10.0.20348.0 (documentation says >= 10.0.19041, but configure complains that it must be >= 10.0.20348.0).
    winsdk_min_vers = (10, 0, 20348, 0)
    winsdk_min_str = '.'.join(map(str, winsdk_min_vers))
    logging.info(">> Checking for Windows 10 SDK >= {}".format(winsdk_min_str))
    # WindowsSDKVersion may have a trailing '\' character
    windowsSdkVersionEnv = os.environ["WindowsSDKVersion"].split('\\')[0]
    winsdk_version = tuple(map(int, windowsSdkVersionEnv.split('.')))
    if winsdk_version < winsdk_min_vers:
        logging.critical("Windows SDK is < {} [{}]".format(winsdk_min_str, windowsSdkVersionEnv))
        return False
    logging.info("Found suitable Windows SDK version >= {} [{}]".format(winsdk_min_str, windowsSdkVersionEnv))

    # Need perl to run qt5/init-repository
    logging.info(">> Checking for perl.exe")
    try:
        res = subprocess.check_output(args="where perl.exe", env=os.environ.copy()).decode().strip()
        logging.info("Found perl.exe [{}]".format(res))
    except subprocess.CalledProcessError:
        logging.critical("Could not locate perl.exe")
        return False

    # CMake >= 3.19 for Qt with QtWebEngine
    logging.info(">> Checking CMake >= 3.19")
    if not checkVersion(vers_cmd="cmake.exe --version",
                        vers_regex="[0-9]*\.[0-9]*\.[0-9]*",
                        min_vers=(3, 19)):
        return False

    # Need node.js version 12 or later for QtWebEngine
    logging.info(">> Checking for node.js version >= 12")
    try:
        res = subprocess.check_output(args="where node.exe", env=os.environ.copy()).decode().strip()
        logging.info("Found node.exe [{}]".format(res))
        # version format: v20.9.0
        if not checkVersion(vers_cmd="node.exe --version",
                            vers_regex="[0-9]*\.[0-9]*\.[0-9]*",
                            min_vers=(12, 0)):
            return False
    except subprocess.CalledProcessError:
        logging.critical("Could not locate node.exe")
        return False

    # QtWebEngine needs python html5lib package
    logging.info(">> Checking pip.exe for html5lib")
    try:
        res = subprocess.check_output(args="where pip.exe", env=os.environ.copy()).decode().strip()
        logging.info("Found pip.exe [{}]".format(res))
        res = subprocess.check_output(args="pip.exe show html5lib", env=os.environ.copy()).decode().strip()
        logging.info("Found html5lib [{}]".format(res))
    except subprocess.CalledProcessError:
        logging.critical("Pyhton package html5lib not found. Please run `pip install html5lib` to install.")
        return False

    # QtWebEngine requires GNUWin32 dependencies gperf, bison, flex
    # TODO(joshuaduong): Locally I installed bison from https://gnuwin32.sourceforge.net/packages.html, but maybe we use chocolatey on the buildbot.
    gnuwin32_deps = ["gperf.exe", "bison.exe", "flex.exe"]
    for dep in gnuwin32_deps:
        logging.info(">> Checking for {}".format(dep))
        try:
            res = subprocess.check_output(args="where {}".format(dep), env=os.environ.copy()).decode().strip()
            logging.info("Found {} [{}]".format(dep, res))
        except subprocess.CalledProcessError:
            logging.critical("{dep} not found. Make sure {dep} is on your PATH.".format(dep=dep))
            return False

    return True

def flattenQtRepo(srcdir):
    """Checks out the given git submodules in the Qt repository.

    The Qt git repository is using git submodules, and relies on a script, `init-repository` to check out
    the git submodules.

    Args:
        srcdir (StrOrBytesPath): Path to the Qt source code.
    """
    logging.info("[{}]>> CMD: perl.exe init-repostory")
    # We might need to retry this if a network error occurred while cloning the git repos
    try:
        # We need to set this git config so init-repository pulls down the git submodules correctly.
        # We might need to mirror these submodules as well.
        subprocess.run(args=["git", "config", "remote.origin.url", "https://github.com/qt/"],
                       cwd=srcdir, env=os.environ.copy())
        res = subprocess.run(args=["perl.exe", "init-repository", "-f"], cwd=srcdir, env=os.environ.copy())
        if res.returncode != 0:
            logging.critical("init-repository exited with non-zero code ({})".format(res.returncode))
            exit(res.returncode)
        logging.debug("[{}]>> CMD: perl.exe init-repostory succeeded".format(srcdir))
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
        os.remove(builddir)
    os.mkdir(builddir)
    conf_args = ["{}\\configure.bat".format(srcdir),
                 "-opensource",
                 "-confirm-license",
                 "-release",
                 "-force-debug-info",
                 "-shared",
                 "-no-rpath",
                 "-nomake", "examples",
                 "-nomake", "tests",
                 "-no-strip",
                 "-qtlibinfix", "AndroidEmu",
                 "-prefix", installdir]
    logging.info("[{}] Running configure.bat".format(builddir))
    logging.info(conf_args)
    res = subprocess.run(args=conf_args, cwd=builddir, env=os.environ.copy())
    if res.returncode != 0:
        logging.critical("configure.bat exited with non-zero code ({})".format(res.returncode))
        exit(res.returncode)
    logging.info("configure.bat succeeded")


def buildQt(submodules, builddir):
    """Builds the specified Qt submodules.

    Args:
        submodules (tuple(str)): The Qt submodules to build.
        builddir (str): The location of the Qt build directory.
    """
    for mod in submodules:
        cmake_build_cmd = "cmake.exe --build . --target {} --parallel".format(mod)
        logging.info(">> " + cmake_build_cmd)
        res = subprocess.run(args=cmake_build_cmd, cwd=builddir, env=os.environ.copy())
        if res.returncode != 0:
            logging.critical("[{}] failed ({})".format(cmake_build_cmd, res.returncode))
            exit(res.returncode)
    logging.info("Build succeeded")

def installQt(submodules, builddir, installdir):
    """Installs the specified Qt submodules to `installdir`.

    Args:
        submodules (tuple(str)): The Qt submodules to install.
        builddir (str): The location of the Qt build directory.
        installdir (str): The location of the Qt install directory.
    """
    logging.info("Installing Qt to {}".format(installdir))
    for mod in submodules:
        cmake_install_cmd = "cmake.exe --install {} --prefix {}".format(mod, installdir)
        logging.info(">> " + cmake_install_cmd)
        res = subprocess.run(args=cmake_install_cmd, cwd=builddir, env=os.environ.copy())
        if res.returncode != 0:
            logging.critical("[{}] failed ({})".format(cmake_install_cmd, res.returncode))
            exit(res.returncode)
    logging.info("Installation succeeded")

def cleanup():
    if os.path.exists(QT_TMP_LOCATION):
        os.remove(QT_TMP_LOCATION)

def buildPrebuilt(args, prebuilts_out_dir):
    atexit.register(cleanup) 

    qt_install_dir = os.path.join(prebuilts_out_dir, "qt")

    # Use cmake from our prebuilts
    os.environ["PATH"] = CMAKE_PATH + ";" + os.environ["PATH"]
    logging.info(os.environ)
    if not checkDependencies():
        logging.fatal("Build environment does not have the required dependencies to build. Exiting..")
        exit(-1)

    # Qt source code is in external/qt5
    if not os.path.isdir(QT_SRC_PATH):
        logging.fatal("{} does not exist".format(QT_SRC_PATH))
        exit(-1)
    logging.info("QT source: {}".format(QT_SRC_PATH))

    # TODO(joshuaduong): If we decide to flatten the repository at check-in, then this step won't be necessary.
    flattenQtRepo(QT_SRC_PATH)

    # We must make sure Qt source code is in a very short directory, otherwise we may
    # get a compiler error because of long paths issue.
    if os.path.exists(QT_TMP_LOCATION):
        os.remove(QT_TMP_LOCATION)
    logging.info("Creating Qt temp directory [{}]".format(QT_TMP_LOCATION))
    os.mkdir(QT_TMP_LOCATION)
    logging.info("Moving source directory to shorter path [{} ==> {}]".format(QT_SRC_PATH, QT_SRC_SHORT_PATH))
    os.rename(QT_SRC_PATH, QT_SRC_SHORT_PATH)

    configureQtBuild(QT_SRC_SHORT_PATH, QT_BUILD_PATH, qt_install_dir)

    buildQt(QT_SUBMODULES, QT_BUILD_PATH)

    installQt(QT_SUBMODULES, QT_BUILD_PATH, qt_install_dir)

    logging.info("Successfully built Qt!")
