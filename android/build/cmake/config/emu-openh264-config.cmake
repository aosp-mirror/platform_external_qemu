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
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/openh264/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

set(OPENH264_INCLUDE_DIR "${PREBUILT_ROOT}/include/wels/")
set(OPENH264_INCLUDE_DIRS "${OPENH264_INCLUDE_DIR}")
set(OPENH264_LIBRARIES "-L${PREBUILT_ROOT}/lib/")

# TODO: account for different OS

set(OPENH264_FOUND TRUE)

if(ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  list(APPEND OPENH264_LIBRARIES "-lopenh264")
endif()

if(NOT TARGET OPENH264::OPENH264)
    add_library(OPENH264::OPENH264 INTERFACE IMPORTED GLOBAL)
    set_target_properties(OPENH264::OPENH264 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OPENH264_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${OPENH264_LIBRARIES}"
    )
endif()



set(
  PACKAGE_EXPORT
  "OPENH264_INCLUDE_DIR;OPENH264_INCLUDE_DIRS;OPENH264_LIBRARIES;OPENH264_FOUND"
  )

