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
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/breakpad/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

set(BREAKPAD_INCLUDE_DIR "${PREBUILT_ROOT}/include/breakpad")
set(BREAKPAD_INCLUDE_DIRS "${BREAKPAD_INCLUDE_DIR}")
set(BREAKPAD_LIBRARIES "${PREBUILT_ROOT}/lib/libbreakpad${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libdisasm${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(BREAKPAD_CLIENT_LIBRARIES "${PREBUILT_ROOT}/lib/libbreakpad_client${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(BREAKPAD_FOUND TRUE)

if(ANDROID_TARGET_TAG STREQUAL "windows-x86_64")
  set(BREAKPAD_DUMPSYM_EXE "${PREBUILT_ROOT}/bin/dump_syms_dwarf")
else()
  set(BREAKPAD_DUMPSYM_EXE "${PREBUILT_ROOT}/bin/dump_syms")
endif()

set(PACKAGE_EXPORT
    "BREAKPAD_INCLUDE_DIR;BREAKPAD_INCLUDE_DIRS;BREAKPAD_LIBRARIES;BREAKPAD_CLIENT_LIBRARIES;BREAKPAD_FOUND;BREAKPAD_DUMPSYM_EXE")
