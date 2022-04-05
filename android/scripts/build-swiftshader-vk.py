#!/usr/bin/env python3

# Copyright 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import glob
import logging
import os
import platform
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

logging.basicConfig(level=logging.INFO)

AOSP_ROOT = os.path.join(os.path.abspath(
    os.path.dirname(__file__)), "..", "..", "..", "..")
PREBUILTS_CMAKE_DIR = os.path.join(AOSP_ROOT, "prebuilts", "cmake")
QEMU_DIR = os.path.join(AOSP_ROOT, "external", "qemu")


def run_win_bash(cmd, cwd):
    git_bash_exe = os.path.join(
        os.environ['PROGRAMFILES'], "Git", "git-bash.exe")
    print(subprocess.run([git_bash_exe, "-c", cmd], cwd=cwd, check=True))


class BuildConfig:
    SUPPORTED_TARGETS = ["linux-x86_64", "darwin-x86_64",
                         "darwin-aarch64", "windows_msvc-x86_64"]

    def __init__(self, target, cmake_args):
        if target not in BuildConfig.SUPPORTED_TARGETS:
            logging.critical("Unknown target: [%s]" % target)
            sys.exit(1)
        self.target = target
        self.shared_lib_prefix = "lib"
        if "linux" in target:
            self.shared_lib_suffix = ".so"
        elif "windows" in target:
            self.shared_lib_suffix = ".dll"
            self.shared_lib_prefix = ""
        elif "darwin" in target:
            self.shared_lib_suffix = ".dylib"
        self.cmake_args = cmake_args


def setup_build_env():
    system = platform.system()
    machine = platform.machine()
    cmake_args = []
    if system == "Linux":
        target = "linux-x86_64"
    elif system == "Darwin":
        if machine == "x86_64":
            target = "darwin-x86_64"
        elif machine == "arm64":
            target = "darwin-aarch64"
        else:
            logging.critical("Unknown machine [%s, %s]" % (system, machine))
            sys.exit(1)
    elif system == "Windows":
        # Check for git-bash
        git_bash_exe = os.path.join(
            os.environ['PROGRAMFILES'], "Git", "git-bash.exe")
        if not Path(git_bash_exe).is_file():
            logging.critical("Building on Windows requires git-bash.exe.")
        # Use the cmake installed in program files by default. Swiftshader wants python3, and our
        # prebuilts cmake 3.18 can only detect up to python 3.9.
        cmake_exe = os.path.join(
            os.environ['PROGRAMFILES'], "CMake", "bin", "cmake.exe")
        if not Path(cmake_exe).is_file():
            logging.info("Unable for find %s. Using cmake on PATH" % cmake_exe)
        else:
            os.environ['PATH'] = "%s;%s" % (
                os.path.dirname(cmake_exe), os.environ['PATH'])
        logging.info("PATH=%s" % os.environ['PATH'])
        target = "windows_msvc-x86_64"
    else:
        logging.critical("Unknown host OS: " + platform.system())
        sys.exit(1)
    return BuildConfig(target, cmake_args)


def clone_repo(dir):
    git_sha1 = "0a1985c2b294e5d6690978bd49eae39042cf52a8"
    git_branch = "master"
    git_dir_name = "SwiftShader"
    git_repo_dir = dir + os.path.sep + git_dir_name

    logging.info("Cloning swiftshader repo at " + git_sha1)
    print(subprocess.run(["git", "clone", "https://swiftshader.googlesource.com/SwiftShader",
          "-b", git_branch, "--single-branch", "--no-checkout"], cwd=dir, check=True).stdout)
    logging.info("Checking out at " + git_sha1)
    print(subprocess.run(["git", "checkout", git_sha1],
          cwd=git_repo_dir, check=True))

    return git_repo_dir


def build_repo(dir, build_config):
    logging.info("Building " + dir)
    print(subprocess.run(["git", "submodule", "update", "--init",
          "--recursive", "third_party/git-hooks"], cwd=dir, check=True))
    if platform.system() == "Windows":
        run_win_bash("./third_party/git-hooks/install_hooks.sh", dir)
    else:
        print(subprocess.run(
            ["./third_party/git-hooks/install_hooks.sh"], cwd=dir, check=True))
    build_dir = dir + os.path.sep + "build"
    toolchain_file = os.path.join(
        QEMU_DIR, "android", "build", "cmake", "toolchain-%s.cmake" % build_config.target)

    if platform.system() == "Windows":
        cmake_cxx_flags = ""
        num_jobs = "/m"
        lib_prefix = ""
    else:
        # -Wno-deprecated-declarations because swiftshader is still depending on
        # c++17-deprecated operations, and the compiler we use (clang-14) enforces it.
        cmake_cxx_flags = "-DCMAKE_CXX_FLAGS=-Wno-deprecated-declarations"
        num_jobs = "-j16"
        lib_prefix = "lib"

    print(subprocess.run(["cmake", "-DCMAKE_BUILD_TYPE=RelWithDebInfo", cmake_cxx_flags,
          "-DCMAKE_TOOLCHAIN_FILE=%s" % toolchain_file, ".", ".."], cwd=build_dir, check=True))

    print(subprocess.run(["cmake", "--build", ".", "--target",
          "vk_swiftshader", "--", num_jobs], cwd=build_dir, check=True))

    logging.info("Done building vk_swiftshader.")
    prebuilts_dir = os.path.join(
        AOSP_ROOT, "prebuilts", "android-emulator-build", "common", "vulkan", build_config.target, "icds")

    out_dir = build_dir + os.path.sep + platform.system()
    out_files = [build_config.shared_lib_prefix + "vk_swiftshader" +
                 build_config.shared_lib_suffix, "vk_swiftshader_icd.json"]

    for f in out_files:
        logging.info("Copying " + os.path.join(out_dir, f) +
                     " ==> " + prebuilts_dir)
        shutil.copy(os.path.join(out_dir, f), prebuilts_dir)


with tempfile.TemporaryDirectory(prefix="swiftshader-vk-") as temp_dir:
    logging.info("Created build directory: " + temp_dir)

    build_config = setup_build_env()
    git_dir = clone_repo(temp_dir)
    build_repo(git_dir, build_config)
