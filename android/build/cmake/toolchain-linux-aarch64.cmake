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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(AOSP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.."
                       ABSOLUTE)
set(ANDROID_QEMU2_TOP_DIR "${AOSP_ROOT}/external/qemu")
include(toolchain)
include(toolchain-rust)


# First we setup all the tags and configure the toolchain
toolchain_configure_tags("linux-aarch64")
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
    internal_set_env_cache(RUNTIME_OS_PROPERTIES "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64:$ORIGIN' -Wl,--disable-new-dtags")
endif()

# We don't want to do any package resolution
set(CMAKE_FIND_ROOT_PATH  "${ANDROID_SYSROOT}")

# Configure how we strip executables.
set(CMAKE_STRIP_CMD "${CMAKE_STRIP} -s")

# And the asm type if we are compiling with ASM
set(ANDROID_ASM_TYPE elf64)

# Next we configure rust.
get_rust_version(RUST_VER)
# configure_rust(COMPILER_ROOT "${AOSP_ROOT}/prebuilts/rust/linux-x86/${RUST_VER}/bin")

