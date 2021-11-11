# Copyright 2021 The Android Open Source Project
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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/libx11-xcb/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

# This is only used for Linux.
if(ANDROID_TARGET_TAG MATCHES "linux.*")
  set(X11_XCB_INCLUDE_DIR "${PREBUILT_ROOT}/include")
  set(X11_XCB_INCLUDE_DIRS "${X11_XCB_INCLUDE_DIR}")
  set(X11_XCB_LIBRARIES
      "${PREBUILT_ROOT}/lib/libX11-xcb${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(X11_XCB_FOUND TRUE)

  android_add_prebuilt_library(
    PACKAGE X11_XCB
    MODULE X11_XCB LOCATION "${PREBUILT_ROOT}/lib/libX11-xcb"
    INCLUDES "${PREBUILT_ROOT}/include"
    LIBNAME libx11-xcb
    URL "https://android.googlesource.com/platform/prebuilts/android-emulator-build/archive/+/refs/heads/emu-master-dev/libX11-1.6.2.tar.bz2"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LGPLv3"
    NOTICE "https://github.com/freedesktop/xorg-libX11/blob/master/COPYING"
    LICENSE LGPL-3.0-only)
else()
  set(X11_XCB_INCLUDE_DIR "")
  set(X11_XCB_INCLUDE_DIRS "")
  set(X11_XCB_LIBRARIES "")
  set(X11_XCB_FOUND FALSE)
endif()

set(PACKAGE_EXPORT "X11_XCB_INCLUDE_DIR;X11_XCB_INCLUDE_DIRS;X11_XCB_LIBRARIES;X11_XCB_FOUND")
