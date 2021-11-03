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

# The toolchain files get processed multiple times during compile detection keep these files simple, and do not setup
# libraries or anything else. merely tags, compiler toolchain and flags should be set here.

get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
include(toolchain)

# First we setup all the tags.
toolchain_configure_tags("darwin-x86_64")

# Make sure we always set the rpath to include current dir and ./lib64. We need this to create self contained
# executables that dynamically load dylibs.
# TODO(joshuaduong): Only add INSTALL_RPATH>=@executable_path/../../../lib64/qt/lib to executables that link against
# the Qt libraries.
set(RUNTIME_OS_PROPERTIES
    "INSTALL_RPATH>=@executable_path/../../../lib64/qt/lib;INSTALL_RPATH>=@loader_path;INSTALL_RPATH>=@loader_path/lib64;BUILD_WITH_INSTALL_RPATH=ON;INSTALL_RPATH_USE_LINK_PATH=ON"
)

if(NOT APPLE)
  get_osxcross_settings("darwin-x86_64")
  toolchain_generate("darwin-x86_64")
  set(CMAKE_SYSTEM_NAME "Darwin")
  set(CMAKE_SYSTEM_PROCESSOR "x86_64")

  # search for programs in the build host directories
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  # for libraries and headers in the target directories
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
  # where is the target environment
  set(CMAKE_FIND_ROOT_PATH
      "${OSXCROSS_SDK}"
      "${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local")
  set(ENV{PKG_CONFIG_LIBDIR} "${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local/lib/pkgconfig")
  set(ENV{PKG_CONFIG_SYSROOT_DIR} "${OSXCROSS_TARGET_DIR}/macports/pkgs")

  set(CMAKE_INSTALL_DO_STRIP TRUE)
else()

  toolchain_generate("darwin-x86_64")
  # Configure how we strip executables.
  set(CMAKE_STRIP_CMD "${CMAKE_STRIP} -S")

endif()

# Always consider the source to be darwin.
add_definitions(-D_DARWIN_C_SOURCE=1)


# And the asm type if we are compiling with ASM
set(ANDROID_ASM_TYPE macho64)
# No magical includes or dependencies for darwin..
