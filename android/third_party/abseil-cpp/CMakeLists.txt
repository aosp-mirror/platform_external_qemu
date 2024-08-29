#
# Copyright 2017 The Abseil Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Most widely used distributions have cmake 3.5 or greater available as of March
# 2019.  A notable exception is RHEL-7 (CentOS7).  You can install a current
# version of CMake by first installing Extra Packages for Enterprise Linux
# (https://fedoraproject.org/wiki/EPEL#Extra_Packages_for_Enterprise_Linux_.28EPEL.29)
# and then issuing `yum install cmake3` on the command line.
if (TARGET absl::strings)
  # Do not include twice.
  return()
endif()
cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0079 NEW)
set(ABSL_ROOT ${ANDROID_QEMU2_TOP_DIR}/../abseil-cpp)
list(APPEND CMAKE_MODULE_PATH ${ABSL_ROOT}/CMake ${ABSL_ROOT}/absl/copts)
set(CMAKE_CXX_STANDARD 17)
set(_BUILD_TESTING ${BUILD_TESTING})
set(BUILD_TESTING OFF)
if(ANDROID_EMULATOR_BUILD AND NOT IN_CROSS_COMPILE)
  set(BUILD_SHARED_LIBS ON)
endif()
include(CMakePackageConfigHelpers)
include(AbseilDll)
include(AbseilHelpers)

# Fix some cross compile settings
if(LINUX_AARCH64)
  set(ABSL_RANDOM_RANDEN_COPTS "${ABSL_RANDOM_HWAES_ARM64_FLAGS}")
elseif(WINDOWS_MSVC_X86_64)
  set(ABSL_RANDOM_RANDEN_COPTS "${ABSL_RANDOM_HWAES_MSVC_X64_FLAGS}")
endif()

#
# Using absl targets
#
# all public absl targets are exported with the absl:: prefix
#
# e.g absl::base absl::synchronization absl::strings ....
#
# DO NOT rely on the internal targets outside of the prefix include current path
list(APPEND ABSL_COMMON_INCLUDE_DIRS ${ABSL_ROOT})

# Build abseil as a shared library
set(ABSL_USING_CLANG ON)
cmake_policy(SET CMP0079 NEW)

# Make sure the dll ends up in the right place.
if(WINDOWS_MSVC_X86_64)
  set(CMAKE_INSTALL_BINDIR "")
  set(CMAKE_INSTALL_LIBDIR "")
else()
  set(ABSL_BUILD_DLL ON)
  set(CMAKE_INSTALL_BINDIR "lib64")
  set(CMAKE_INSTALL_LIBDIR "lib64")
endif()

add_subdirectory(${ABSL_ROOT}/absl absl)
if(DARWIN_X86_64 OR DARWIN_AARCH64)
  target_link_libraries(abseil_dll PUBLIC "-framework CoreFoundation")
endif()

if(LINUX_AARCH64 OR LINUX_X86_64)
  target_link_libraries(abseil_dll PUBLIC "-pthread")
endif()
set(BUILD_TESTING ${_BUILD_TESTING})

android_license(
  TARGET abseil_dll
  LIBNAME abseil
  URL "https://android.googlesource.com/platform/external/abseil-cpp/+/refs/heads/emu-master-dev"
  SPDX "Apache-2"
  LICENSE
    "https://android.googlesource.com/platform/external/abseil-cpp/+/refs/heads/emu-master-dev/LICENSE"
  LOCAL "${ABSL_ROOT}/LICENSE")
