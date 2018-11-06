# Copyright 2018 The Android Open Source Project
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
get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
# cmake will look for WinMSVCCrossCompile in Platform/WinMSVCCrossCompile.cmake
list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}/Modules")
set(CMAKE_SYSTEM_NAME WinMSVCCrossCompile)

include(toolchain)

# First we create the toolchain
get_host_tag(ANDROID_HOST_TAG)
set (ANDROID_TARGET_TAG "windows_msvc-x86_64")
set (ANDROID_TARGET_OS "windows_msvc")
set (ANDROID_TARGET_OS_FLAVOR "windows")
set(CMAKE_SYSTEM_PROCESSOR AMD64)

get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_FILE}/../../../../" ABSOLUTE)

# Cmake goes crazy if we set AR manually.. so let's not do that.
toolchain_generate("${ANDROID_TARGET_TAG}")
# need to set ar to llvm-ar to unpack windows static libs
set(CMAKE_AR ${COMPILER_PREFIX}ar)

# here is the target environment located, used to
# locate packages. We don't want to do any package resolution
# with windows-msvc, so we explicitly disable it.
set(CMAKE_FIND_ROOT_PATH  "${ANDROID_SYSROOT}")

# Disable any searching as it might lead to unexpected behavior
# that varies amongst build environments
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
