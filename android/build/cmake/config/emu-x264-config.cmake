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
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/x264/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

set(X264_INCLUDE_DIR "${PREBUILT_ROOT}/include/")
set(X264_INCLUDE_DIRS "${X264_INCLUDE_DIR}")
set(X264_LIBRARIES "${PREBUILT_ROOT}/lib/libx264${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(X264_FOUND TRUE)

android_add_prebuilt_library(X264 X264 "${PREBUILT_ROOT}/lib/libvpx" "${PREBUILT_ROOT}/include" "" "")

set(PACKAGE_EXPORT "X264_INCLUDE_DIR;X264_INCLUDE_DIRS;X264_LIBRARIES;X264_FOUND")
