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

logging.basicConfig(level=logging.INFO)

def setup_build_env():
    system = platform.system()
    if system == "Linux":
            # Make sure clang (clang-11 known to work) is the compiler used. gcc doesn't work
            search_path = "/usr/bin/"
            preferred_version = "11"
            preferred_cxx = search_path + "clang++-" + preferred_version;
            clang_bins = glob.glob(search_path + "/clang++-*", recursive=False)
            if not clang_bins:
                logging.critical("Unable to locate clang in " + search_path)
                sys.exit(1)

            # use clang-11 if available
            if preferred_cxx in clang_bins:
                cxx = preferred_cxx
            else:
                # Use latest version
                latest_version = 0
                for c in clang_bins:
                    v = int(c.split("++-")[1])
                    if v > latest_version:
                        latest_version = v
                        cxx = c
            cc = cxx.replace("+", "")
            logging.info("Using compiler CXX=" + cxx + " CC=" + cc)
            os.environ['CC'] = cc;
            os.environ['CXX'] = cxx;
            return "linux-x86_64"
    else:
            logging.critical("Unknown host OS: " + platform.system())
            sys.exit(1)

def clone_repo(dir):
    git_sha1 = "ae82a91bf47a9c4baa36a423057ddbcfb032beb6"
    git_branch = "master"
    git_dir_name = "SwiftShader"
    git_repo_dir = dir + os.path.sep + git_dir_name

    logging.info("Cloning swiftshader repo at " + git_sha1)
    print(subprocess.run(["git", "clone", "https://swiftshader.googlesource.com/SwiftShader", "-b", git_branch,
                          "--single-branch", "--no-checkout"], cwd=dir, check=True, capture_output=False).stdout)
    logging.info("Checking out at " + git_sha1)
    print(subprocess.run(["git", "checkout", git_sha1], cwd=git_repo_dir, check=True, capture_output=False))

    logging.info("Applying patch https://swiftshader-review.googlesource.com/c/SwiftShader/+/62228")
    print(subprocess.run(["git", "fetch", "https://swiftshader.googlesource.com/SwiftShader",
                          "refs/changes/28/62228/2"], cwd=git_repo_dir, check=True, capture_output=False))
    print(subprocess.run(["git", "cherry-pick", "FETCH_HEAD"],
                         cwd=git_repo_dir, check=True, capture_output=False))
    return git_repo_dir

def build_repo(dir, target):
    logging.info("Building " + dir)
    print(subprocess.run(["git", "submodule", "update", "--init", "--recursive", "third_party/git-hooks"],
                         cwd=dir, check=True, capture_output=False))
    print(subprocess.run(["./third_party/git-hooks/install_hooks.sh"],
                         cwd=dir, check=True, capture_output=False))
    build_dir = dir + os.path.sep + "build"
    print(subprocess.run(["cmake", ".."], cwd=build_dir, check=True, capture_output=False))
    print(subprocess.run(["cmake", "--build", ".", "--target", "vk_swiftshader", "--", "-j16"],
                         cwd=build_dir, check=True, capture_output=False))

    logging.info("Done building vk_swiftshader.")
    prebuilts_dir = os.path.join(os.path.dirname(__file__), "..", "..", "..", "..", "prebuilts",
                                 "android-emulator-build", "common", "vulkan", target, "icds")

    if platform.system() == "Linux":
        out_dir = build_dir + os.path.sep + "Linux"
        out_files = ["libvk_swiftshader.so", "vk_swiftshader_icd.json"]
    else:
        logging.critical("Unknown system. Please implement this for system=" + platform.system())

    for f in out_files:
        logging.info("Copying " + os.path.join(out_dir, f) + " ==> " + prebuilts_dir)
        shutil.copy(os.path.join(out_dir, f), prebuilts_dir)

with tempfile.TemporaryDirectory(prefix="swiftshader-vk-") as temp_dir:
    logging.info("Created build directory: " + temp_dir)

    target = setup_build_env()
    git_dir = clone_repo(temp_dir)
    build_repo(git_dir, target)
