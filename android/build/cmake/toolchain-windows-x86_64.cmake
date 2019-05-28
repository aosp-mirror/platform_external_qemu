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

# The toolchain files can get processed multiple times during compile detection
# keep these files simple, and do not setup libraries or anything else.
# merely tags, compiler toolchain and flags should be set here.

get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
include(toolchain)

# First we setup all the tags.
toolchain_configure_tags("windows-x86_64")

SET(CMAKE_SYSTEM_NAME Windows)
get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_FILE}/../../../../" ABSOLUTE)

# Cmake goes crazy if we set AR manually.. so let's not do that.
toolchain_generate("windows-x86_64")

list(APPEND RUNTIME_OS_DEPENDENCIES "${ANDROID_SYSROOT}/bin/libwinpthread-1.dll>lib64/libwinpthread-1.dll")
list(APPEND RUNTIME_OS_DEPENDENCIES "${ANDROID_SYSROOT}/bin/libwinpthread-1.dll>lib64/../libwinpthread-1.dll")
list(APPEND RUNTIME_OS_DEPENDENCIES "${ANDROID_SYSROOT}/lib/libgcc_s_seh-1.dll>lib64/libgcc_s_seh-1.dll")
list(APPEND RUNTIME_OS_DEPENDENCIES "${ANDROID_SYSROOT}/lib/libgcc_s_seh-1.dll>lib64/../libgcc_s_seh-1.dll")

# Make sure we add a build-id section for every compiled component.
add_compile_options(-m64)
add_compile_options(-mcx16)
add_compile_options(-falign-functions)
add_compile_options( -ftracer)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--build-id" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--build-id" CACHE STRING "" FORCE)

# here is the target environment located, used to
# locate packages. We don't want to do any package resolution
# with mingw, so we explicitly disable it.
set(CMAKE_FIND_ROOT_PATH  "${ANDROID_SYSROOT}")

add_definitions(-DWINVER=0x601 -D_WIN32_WINNT=0x601)

# here is the target environment located, used to locate packages. We don't want to do any package resolution with
# mingw, so we explicitly disable it.
set(CMAKE_FIND_ROOT_PATH "${ANDROID_SYSROOT}")

# Disable any searching as it might lead to unexpected behavior that varies amongst build environments
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
set(CMAKE_STRIP_CMD "${CMAKE_STRIP} -s")
set(ANDROID_YASM_TYPE win64)
