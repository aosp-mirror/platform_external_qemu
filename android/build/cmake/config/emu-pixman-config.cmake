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

set(PIXMAN_INCLUDE_DIR "${PREBUILT_ROOT}/include/pixman-1")
set(PIXMAN_INCLUDE_DIRS ${PIXMAN_INCLUDE_DIR})
set(PIXMAN_LIBRARIES -lpixman-1 -L${PREBUILT_ROOT}/lib)
set(PIXMAN_FOUND TRUE)
android_add_prebuilt_library(PIXMAN PIXMAN "${PREBUILT_ROOT}/lib/libpixman-1" "${PREBUILT_ROOT}/include/pixman-1" "" "")

set(PACKAGE_EXPORT "PIXMAN_INCLUDE_DIR;PIXMAN_INCLUDE_DIRS;PIXMAN_LIBRARIES;PIXMAN_FOUND")
