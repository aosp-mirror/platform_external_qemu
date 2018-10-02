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

# Note! This file can get included many times, so we use some tricks to
# Only calculate the settings once.
get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
include(toolchain)

# First we create the toolchain
set(ANDROID_TARGET_TAG "linux-x86_64")
set(ANDROID_TARGET_OS "linux")
get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_FILE}/../../../../" ABSOLUTE)
toolchain_generate("${ANDROID_TARGET_TAG}")

get_env_cache(RUNTIME_OS_PROPERTIES)
get_env_cache(RUNTIME_OS_DEPENDENCIES)
if ("${RUNTIME_OS_DEPENDENCIES}" STREQUAL "")
    toolchain_cmd("${ANDROID_TARGET_TAG}" "--print=libcplusplus" "unused_param")
    get_filename_component(RESOLVED_SO "${STD_OUT}" REALPATH)
    get_filename_component(RESOLVED_FILENAME "${RESOLVED_SO}" NAME)

    set_env_cache(RUNTIME_OS_DEPENDENCIES "${STD_OUT}>lib64/${RESOLVED_FILENAME}")

    # Configure the RPATH be dynamic..
    set_env_cache(RUNTIME_OS_PROPERTIES "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64'")
endif()

# here is the target environment located, used to
# locate packages. We don't want to do any package resolution
# with mingw, so we explicitly disable it.
set(CMAKE_FIND_ROOT_PATH  "${ANDROID_SYSROOT}")
