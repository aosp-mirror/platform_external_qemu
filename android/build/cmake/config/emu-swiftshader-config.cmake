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

set(TARGET_TAG ${ANDROID_TARGET_TAG})
if(NOT WINDOWS_X86_64)
  message(FATAL_ERROR "You should only use the swiftshader prebuilt for mingw")
endif()

get_filename_component(PREBUILT_ROOT
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/e2fsprogs/${TARGET_TAG}"
                       ABSOLUTE)

set(
  SWIFTSHADER_DEPENDENCIES
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-x86_64/lib/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-x86_64/lib/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-x86_64/lib/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )

