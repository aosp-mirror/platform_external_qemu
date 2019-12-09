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
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/libnvidia/${ANDROID_TARGET_TAG}"
  ABSOLUTE)
if(NOT EXISTS "${PREBUILT_ROOT}/include/cuda.h")
  message(FATAL_ERROR "The libnvidia prebuilts are not available for this target. You will have to create them first.")
endif()
if(ANDROID_TARGET_TAG MATCHES "linux.*")
  set(LIBNVIDIA_INCLUDE_DIR "${PREBUILT_ROOT}/include")
  set(LIBNVIDIA_INCLUDE_DIRS ${LIBNVIDIA_INCLUDE_DIR})
  set(LIBNVIDIA_LIBRARIES -lcuda -lnvcuvid -L${PREBUILT_ROOT}/lib)

  add_library(LIBNVIDIA::LIBNVIDIA INTERFACE IMPORTED GLOBAL)
  set_target_properties(LIBNVIDIA::LIBNVIDIA PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${LIBNVIDIA_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${LIBNVIDIA_LIBRARIES}"
  )
endif()
set(LIBNVIDIA_FOUND TRUE)

set(PACKAGE_EXPORT
    "LIBNVIDIA_INCLUDE_DIR;LIBNVIDIA_INCLUDE_DIRS;LIBNVIDIA_LIBRARIES;LIBNVIDIA_FOUND")
