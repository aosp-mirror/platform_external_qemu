# Copyright 2019 The Android Open Source Project
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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/libunwind/${ANDROID_TARGET_TAG}"
  ABSOLUTE)
if(LINUX)
  set(LIBUNWIND_INCLUDE_DIR "${PREBUILT_ROOT}/include")
  set(LIBUNWIND_INCLUDE_DIRS ${LIBUNWIND_INCLUDE_DIR})
  set(LIBUNWIND_LIBRARIES
      ${PREBUILT_ROOT}/lib/libunwind${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/libunwind-x86_64${CMAKE_STATIC_LIBRARY_SUFFIX})
  if(NOT TARGET LIBUNWIND::LIBUNWIND)
    add_library(LIBUNWIND::LIBUNWIND INTERFACE IMPORTED GLOBAL)
    set_target_properties(
      LIBUNWIND::LIBUNWIND
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBUNWIND_INCLUDE_DIRS}"

                 INTERFACE_LINK_LIBRARIES "${LIBUNWIND_LIBRARIES}")
  endif()
endif()
set(LIBUNWIND_FOUND TRUE)

set(PACKAGE_EXPORT
    "LIBUNWIND_INCLUDE_DIR;LIBUNWIND_INCLUDE_DIRS;LIBUNWIND_LIBRARIES;LIBUNWIND_FOUND"
)

android_license(
  TARGET LIBUNWIND::LIBUNWIND
  LIBNAME libunwind
  URL # Fix when we host it internally (b/148256090)
      "https://github.com/libunwind/libunwind/releases/tag/v1.3.1"
  SPDX "MIT"
  LICENSE "https://github.com/libunwind/libunwind/blob/master/COPYING"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LIBUNWIND")
