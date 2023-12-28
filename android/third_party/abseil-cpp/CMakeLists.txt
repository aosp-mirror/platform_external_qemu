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
cmake_minimum_required(VERSION 3.5)
set(ABSL_ROOT ${ANDROID_QEMU2_TOP_DIR}/../abseil-cpp)
list(APPEND CMAKE_MODULE_PATH ${ABSL_ROOT}/CMake ${ABSL_ROOT}/absl/copts)
set(CMAKE_CXX_STANDARD 17)
set(_BUILD_TESTING ${BUILD_TESTING})
set(BUILD_TESTING OFF)
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
set(ABSL_USING_CLANG ON)

add_subdirectory(${ABSL_ROOT}/absl absl)

# Let's not worry/fix abseil warnings
target_compile_options(absl_time PRIVATE -Wno-tautological-type-limit-compare)
target_compile_options(absl_base PUBLIC -Wno-anon-enum-enum-conversion)
target_compile_options(
  absl_time_zone
  PRIVATE -Wno-shadow -Wno-shadow-uncaptured-local
          -Wno-tautological-type-limit-compare -Wno-unused-template)

set(BUILD_TESTING ${_BUILD_TESTING})
