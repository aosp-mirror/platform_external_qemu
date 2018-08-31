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
include(toolchain)

# First we create the toolchain
set (ANDROID_TARGET_TAG "windows-x86_64")
set (ANDROID_TARGET_OS "windows")
set (ANDROID_TARGET_OS_FLAVOR "windows")
#set(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_NAME Windows)
get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_FILE}/../../../../" ABSOLUTE)

# Cmake goes crazy if we set AR manually.. so let's not do that.
toolchain_generate("${ANDROID_TARGET_TAG}")

list(APPEND RUNTIME_OS_DEPENDENCIES "${ANDROID_SYSROOT}/bin/libwinpthread-1.dll>lib64/libwinpthread-1.dll")
list(APPEND RUNTIME_OS_DEPENDENCIES "${ANDROID_SYSROOT}/lib32/libwinpthread-1.dll>lib/libwinpthread-1.dll")
list(APPEND RUNTIME_OS_DEPENDENCIES "${ANDROID_SYSROOT}/lib/libgcc_s_seh-1.dll>lib64/libgcc_s_seh-1.dll")
list(APPEND RUNTIME_OS_DEPENDENCIES "${ANDROID_SYSROOT}/lib32/libgcc_s_sjlj-1.dll>lib/libgcc_s_sjlj-1.dll")

list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-m64 -static-libgcc -Xlinker --build-id -mcx16")

# here is the target environment located, used to
# locate packages. We don't want to do any package resolution
# with mingw, so we explicitly disable it.
set(CMAKE_FIND_ROOT_PATH  "${ANDROID_SYSROOT}")

# Disable any searching as it might lead to unexpected behavior
# that varies amongst build environments
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)


