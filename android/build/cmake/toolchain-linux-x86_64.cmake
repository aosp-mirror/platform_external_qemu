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

# The toolchain files get processed multiple times during compile detection
# keep these files simple, and do not setup libraries or anything else.
# merely tags, compiler toolchain and flags should be set here.

# Note! This file can get included many times, so we use some tricks to
# Only calculate the settings once.
get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
include(toolchain)

# First we setup all the tags and configure the toolchain
toolchain_configure_tags("linux-x86_64")
get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
toolchain_generate("${ANDROID_TARGET_TAG}")

internal_get_env_cache(RUNTIME_OS_PROPERTIES)
internal_get_env_cache(RUNTIME_OS_DEPENDENCIES)
if ("${RUNTIME_OS_DEPENDENCIES}" STREQUAL "")
    toolchain_cmd("${ANDROID_TARGET_TAG}" "--print=libcplusplus" "unused_param")
    get_filename_component(RESOLVED_SO "${STD_OUT}" REALPATH)
    get_filename_component(RESOLVED_FILENAME "${RESOLVED_SO}" NAME)
    get_filename_component(LINKED_FILENAME "${STD_OUT}" NAME)

    internal_set_env_cache(RUNTIME_OS_DEPENDENCIES "${STD_OUT}>lib64/${RESOLVED_FILENAME};${STD_OUT}>lib64/${LINKED_FILENAME}")

    # Configure the RPATH be dynamic..
    #
    # FIXME: Remove --disable-new-dtags. Binutils now uses RUNPATH for
    # searching for shared objects by default, which doesn't apply to
    # transitive shared object dependencies. These shared objects should
    # ideally specify their own rpaths.
    internal_set_env_cache(RUNTIME_OS_PROPERTIES "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64' -Wl,--disable-new-dtags")
endif()

# We don't want to do any package resolution
set(CMAKE_FIND_ROOT_PATH  "${ANDROID_SYSROOT}")

# Configure how we strip executables.
set(CMAKE_STRIP_CMD "${CMAKE_STRIP} -s")

# And the asm type if we are compiling with yasm
set(ANDROID_YASM_TYPE elf64)
