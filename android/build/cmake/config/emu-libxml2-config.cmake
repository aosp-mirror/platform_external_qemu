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
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/libxml2/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

set(LIBXML2_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(LIBXML2_INCLUDE_DIRS "${LIBXML2_INCLUDE_DIR}")
set(LIBXML2_LIBRARIES "${PREBUILT_ROOT}/lib/libxml2${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(LIBXML2_DEFINITIONS "-DLIBXML_STATIC")
set(LIBXML2_FOUND TRUE)

android_add_prebuilt_library(LibXml2 LibXml2 "${PREBUILT_ROOT}/lib/libxml2" "${PREBUILT_ROOT}/include" "LIBXML_STATIC")

set(PACKAGE_EXPORT "LIBXML2_INCLUDE_DIR;LIBXML2_INCLUDE_DIRS;LIBXML2_LIBRARIES;LIBXML2_FOUND;LIBXML2_DEFINITIONS")
