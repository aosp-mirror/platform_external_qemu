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

# Note this is a pseudo toolchain file compiling on windows with VS
if (NOT MSVC)
   message(FATAL_ERROR "This definition should only be used by visual studio.")
endif()

get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}/Modules")

include(toolchain)

# First we configure our variables.
get_host_tag(ANDROID_HOST_TAG)

# We are mostly compatible with windows_msvc
set (ANDROID_TARGET_TAG "windows_msvc-x86_64")
set (ANDROID_HOST_TAG "windows_msvc-x86_64")
set (ANDROID_TARGET_OS "windows_msvc")
set (ANDROID_TARGET_OS_FLAVOR "windows")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -MD")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -MD")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -MD")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -MD")

get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_FILE}/../../../../" ABSOLUTE)

# Make sure nobody is accidently trying to do a 32 bit build.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Please configure the compiler for a x64 build!")
endif()

# Including windows.h will cause issues with std::min/std::max
add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES -DWIN32_LEAN_AND_MEAN)

include(emu-windows-libs)
