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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/tcmalloc/${ANDROID_TARGET_TAG}"
  ABSOLUTE)
if(NOT ANDROID_TARGET_TAG MATCHES "windows.*")
  set(TCMALLOC_INCLUDE_DIR "${PREBUILT_ROOT}/include")
  set(TCMALLOC_INCLUDE_DIRS ${TCMALLOC_INCLUDE_DIR})
  set(TCMALLOC_LIBRARIES -ltcmalloc_minimal -L${PREBUILT_ROOT}/lib)
  if(LINUX)
    set(TCMALLOC_OS_DEPENDENCIES
        "${PREBUILT_ROOT}/lib/libtcmalloc_minimal.so.4>lib64/libtcmalloc_minimal.so.4"
    )
  elseif(DARWIN_X86_64)
    set(TCMALLOC_OS_DEPENDENCIES
        "${PREBUILT_ROOT}/lib/libtcmalloc_minimal.4.dylib>lib64/libtcmalloc_minimal.4.dylib"
    )
  endif()

  android_license(
    TARGET TCMALLOC_OS_DEPENDENCIES
    LIBNAME "tcmalloc"
    URL "https://chromium.googlesource.com/external/github.com/gperftools/gperftools/+/refs/heads/master"
    SPDX "BSD-3-Clause"
    LICENSE
      "https://chromium.googlesource.com/external/github.com/gperftools/gperftools/+/refs/heads/master/COPYING"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.TCMALLOC")

  android_add_prebuilt_library(
    SHARED
    PACKAGE TCMALLOC
    MODULE TCMALLOC LOCATION "${PREBUILT_ROOT}/lib/libtcmalloc_minimal"
    INCLUDES "${PREBUILT_ROOT}/include"
    LIBNAME tcmalloc
    URL "https://chromium.googlesource.com/external/github.com/gperftools/gperftools/+/refs/heads/master"
    LICENSE "BSD-3-Clause"
    NOTICE
      "https://chromium.googlesource.com/external/github.com/gperftools/gperftools/+/refs/heads/master/COPYING"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.TCMALLOC")
endif()
set(TCMALLOC_FOUND TRUE)

set(PACKAGE_EXPORT
    "TCMALLOC_INCLUDE_DIR;TCMALLOC_INCLUDE_DIRS;TCMALLOC_LIBRARIES;TCMALLOC_FOUND;TCMALLOC_OS_DEPENDENCIES"
)
