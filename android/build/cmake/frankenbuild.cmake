# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

# This contains a set of definitions that will make it possible to use the
# upcoming build system by merely switching a flag.

# COMPILER + PATH CONFIGURATION
# =============================
# TODO(jansene): This section is needed as a bridge to the android build system.
if(NOT DEFINED ANDROID_TARGET_TAG)
  set(ANDROID_FRANKENBUILD ON)
  string(REPLACE " "
                 ";"
                 INCLUDES
                 ${LOCAL_C_INCLUDES})

  # Use same compiler config as android build system
  set(CMAKE_C_FLAGS ${LOCAL_CFLAGS})
  set(CMAKE_CXX_FLAGS "${LOCAL_CFLAGS} ${LOCAL_CXXFLAGS}")

  # Work around some gcc/mingw issues
  if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()

  # Make sure we create archives in the same way..
  set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")

  set(ANDROID_QEMU2_TOP_DIR ${LOCAL_QEMU2_TOP_DIR})
  set(ANDROID_TARGET_TAG ${LOCAL_TARGET_TAG})
  set(ANDROID_TARGET_OS ${LOCAL_OS})
  set(ANDROID_TARGET_OS_FLAVOR ${LOCAL_OS_FLAVOR})

  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
