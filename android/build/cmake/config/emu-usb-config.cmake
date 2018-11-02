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
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/libusb/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

# No usb support on windows!
if(NOT ANDROID_TARGET_TAG MATCHES "windows.*")
set(USB_INCLUDE_DIR "${PREBUILT_ROOT}/include/libusb-1.0")
set(USB_INCLUDE_DIRS "${USB_INCLUDE_DIR}")
set(USB_LIBRARIES "${PREBUILT_ROOT}/lib/libusb-1.0${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(USB_FOUND TRUE)

android_add_prebuilt_library(USB USB "${PREBUILT_ROOT}/lib/libusb-1.0" "${PREBUILT_ROOT}/include/libusb-1.0" "" "")

endif()
set(PACKAGE_EXPORT "USB_INCLUDE_DIR;USB_INCLUDE_DIRS;USB_LIBRARIES;USB_FOUND")
