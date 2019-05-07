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
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/webrtc/${ANDROID_TARGET_TAG}"
  ABSOLUTE)
if(NOT EXISTS "${PREBUILT_ROOT}/include/common_types.h")
  if(DARWIN_X86_64)
    message(
      FATAL_ERROR "The webrtc prebuilts are not in the repo by default. You can apply this patch:"
                  "https://android-review.googlesource.com/c/platform/prebuilts/android-emulator-build/common/+/948149"
                  "in the ${PREBUILT_ROOT} directory.")
  else()
    message(FATAL_ERROR "The webrtc prebuilts are not available for this target. You will have to create them first."
                        "See: ${ANDROID_QEMU2_TOP_DIR}/android/scripts/build-webrtc/README.md for details")
  endif()
endif()
android_add_prebuilt_library(WebRTC WebRTC "${PREBUILT_ROOT}/lib/libwebrtc" "${PREBUILT_ROOT}/include" "" "")
