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
if(ANDROID_TARGET_TAG MATCHES "windows_msvc.*")
  set(BREAKPAD_LIBRARIES
      ${PREBUILT_ROOT}/lib/common${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/exception_handler${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/crash_report_sender${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/crash_generation_server${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/processor${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(BREAKPAD_CLIENT_LIBRARIES
      ${PREBUILT_ROOT}/lib/common${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/exception_handler${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/crash_generation_client${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/processor${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/libdisasm${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
  set(BREAKPAD_LIBRARIES
      ${PREBUILT_ROOT}/lib/libbreakpad${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/libdisasm${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(BREAKPAD_CLIENT_LIBRARIES
      ${PREBUILT_ROOT}/lib/libbreakpad_client${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
set(BREAKPAD_FOUND TRUE)

if(ANDROID_TARGET_TAG STREQUAL "windows-x86_64")
  set(BREAKPAD_DUMPSYM_EXE "${PREBUILT_ROOT}/bin/dump_syms_dwarf")
else()
  set(BREAKPAD_DUMPSYM_EXE "${PREBUILT_ROOT}/bin/dump_syms")
endif()

if(NOT TARGET BREAKPAD::Breakpad)
    add_library(BREAKPAD::Breakpad INTERFACE IMPORTED GLOBAL)
    set_target_properties(BREAKPAD::Breakpad PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${BREAKPAD_LIBRARIES}"
    )
    add_library(BREAKPAD::Client INTERFACE IMPORTED GLOBAL)
    set_target_properties(BREAKPAD::Client PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${BREAKPAD_CLIENT_LIBRARIES}"
    )
endif()

set(PACKAGE_EXPORT
    "BREAKPAD_INCLUDE_DIR;BREAKPAD_INCLUDE_DIRS;BREAKPAD_LIBRARIES;BREAKPAD_CLIENT_LIBRARIES;BREAKPAD_FOUND;BREAKPAD_DUMPSYM_EXE")
