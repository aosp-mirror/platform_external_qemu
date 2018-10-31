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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}" ABSOLUTE)

set(ZLIB_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIR}")
set(ZLIB_LIBRARIES "${PREBUILT_ROOT}/lib/libz${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(ZLIB_FOUND TRUE)

android_add_prebuilt_library(ZLIB ZLIB "${PREBUILT_ROOT}/lib/libz" ${ZLIB_INCLUDE_DIRS} "" "")

set(PACKAGE_EXPORT "ZLIB_INCLUDE_DIR;ZLIB_INCLUDE_DIRS;ZLIB_LIBRARIES;ZLIB_FOUND")
