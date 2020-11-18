# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.

get_filename_component(
  PREBUILT_ROOT
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/e2fsprogs/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

if(DARWIN_X86_64 OR DARWIN_AARCH64)
  # Apple does its own thing, the headers should be on the path, so no need to
  # set them explicitly.
  android_license(TARGET UUID::UUID LIBNAME uuid SPDX None LICENSE None
                  LOCAL None)
elseif(LINUX)
  set(UUID_INCLUDE_DIR "${PREBUILT_ROOT}/include")
  set(UUID_INCLUDE_DIRS "${UUID_INCLUDE_DIR}")
  set(UUID_LIBRARIES
      "${PREBUILT_ROOT}/lib/libuuid${CMAKE_STATIC_LIBRARY_SUFFIX}")

  android_add_prebuilt_library(
    PACKAGE UUID
    MODULE UUID LOCATION "${PREBUILT_ROOT}/lib/libuuid"
    INCLUDES "${UUID_INCLUDE_DIR}"
    LIBNAME uuid
    URL "https://android.googlesource.com/platform/prebuilts/android-emulator-build/archive/+/refs/heads/emu-master-dev/libuuid1-2.25.2-2.tar.xz"
    LICENSE "BSD-3-Clause"
    NOTICE "https://spdx.org/licenses/BSD-3-Clause.html"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.UUID")
elseif(WINDOWS)
  if(WINDOWS_MSVC_X86_64)
    set(UUID_LIBRARIES "rpcrt4")
  else()
    set(UUID_LIBRARIES "-lrpcrt4")
  endif()
  android_license(TARGET UUID::UUID LIBNAME uuid SPDX None LICENSE None
                  LOCAL None)
endif()
if(NOT TARGET UUID::UUID)
  add_library(UUID::UUID INTERFACE IMPORTED GLOBAL)
  set_target_properties(
    UUID::UUID
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${UUID_INCLUDE_DIRS}"

               INTERFACE_LINK_LIBRARIES "${UUID_LIBRARIES}")
endif()
set(PACKAGE_EXPORT
    "UUID_INCLUDE_DIR;UUID_INCLUDE_DIRS;UUID_LIBRARIES;UUID_FOUND")
set(UUID_FOUND TRUE)
