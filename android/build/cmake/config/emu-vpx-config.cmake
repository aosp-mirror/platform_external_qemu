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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/libvpx/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

set(VPX_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(VPX_INCLUDE_DIRS "${VPX_INCLUDE_DIR}")
set(VPX_LIBRARIES "${PREBUILT_ROOT}/lib/libvpx${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(VPX_FOUND TRUE)

if (LINUX_X86_64)
android_add_prebuilt_library(
  PACKAGE VPX
  MODULE VPX LOCATION "${PREBUILT_ROOT}/lib/libvpx"
  INCLUDES "${VPX_INCLUDE_DIR}"
  LIBNAME libvpx
  ALIAS libvpx
  URL "https://chromium.googlesource.com/webm/libvpx/"
  LICENSE "BSD-3-Clause"
  NOTICE
    "https://chromium.googlesource.com/webm/libvpx/+/refs/heads/master/LICENSE"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.WEBM")
else()
android_add_prebuilt_library(
  PACKAGE VPX
  MODULE VPX LOCATION "${PREBUILT_ROOT}/lib/libvpx"
  INCLUDES "${VPX_INCLUDE_DIR}"
  LIBNAME libvpx
  URL "https://chromium.googlesource.com/webm/libvpx/"
  LICENSE "BSD-3-Clause"
  NOTICE
    "https://chromium.googlesource.com/webm/libvpx/+/refs/heads/master/LICENSE"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.WEBM")
endif()
set(PACKAGE_EXPORT "VPX_INCLUDE_DIR;VPX_INCLUDE_DIRS;VPX_LIBRARIES;VPX_FOUND")
