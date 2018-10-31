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
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/tcmalloc/${ANDROID_TARGET_TAG}"
  ABSOLUTE)
if(ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  set(TCMALLOC_INCLUDE_DIR "")
  set(TCMALLOC_INCLUDE_DIRS ${TCMALLOC_INCLUDE_DIR})
  set(TCMALLOC_LIBRARIES -ltcmalloc_minimal -L${PREBUILT_ROOT}/lib64)
  set(TCMALLOC_OS_DEPENDENCIES "${PREBUILT_ROOT}/lib64/libtcmalloc_minimal.so.4>lib64/libtcmalloc_minimal.so.4")
endif()
set(TCMALLOC_FOUND TRUE)

set(PACKAGE_EXPORT
    "TCMALLOC_INCLUDE_DIR;TCMALLOC_INCLUDE_DIRS;TCMALLOC_LIBRARIES;TCMALLOC_FOUND;TCMALLOC_OS_DEPENDENCIES")
