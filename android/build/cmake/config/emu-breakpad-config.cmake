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
get_filename_component(
  HOST_PREBUILT_ROOT
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/breakpad/${ANDROID_HOST_TAG}" ABSOLUTE)

set(BREAKPAD_INCLUDE_DIR "${PREBUILT_ROOT}/include/breakpad")
set(BREAKPAD_INCLUDE_DIRS "${BREAKPAD_INCLUDE_DIR}")
if(WINDOWS_MSVC_X86_64)
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
  if(WINDOWS_X86_64)
    set(BREAKPAD_DUMPSYM_EXE "${HOST_PREBUILT_ROOT}/bin/dump_syms_dwarf")
    set(BREAKPAD_MINIDUMP_STACKWALK_EXE "${PREBUILT_ROOT}/bin/minidump_stackwalk.exe")
  else()
    set(BREAKPAD_DUMPSYM_EXE "${HOST_PREBUILT_ROOT}/bin/dump_syms")
    set(BREAKPAD_MINIDUMP_STACKWALK_EXE "${HOST_PREBUILT_ROOT}/bin/minidump_stackwalk")
  endif()
  set(BREAKPAD_LIBRARIES ${PREBUILT_ROOT}/lib/libbreakpad${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${PREBUILT_ROOT}/lib/libdisasm${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(BREAKPAD_CLIENT_LIBRARIES ${PREBUILT_ROOT}/lib/libbreakpad_client${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
set(BREAKPAD_FOUND TRUE)

set(BREAKPAD_UPLOAD_EXE "${HOST_PREBUILT_ROOT}/bin/sym_upload")

if(NOT TARGET breakpad_server)
  add_library(breakpad_server INTERFACE IMPORTED GLOBAL)
  set_target_properties(breakpad_server
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_INCLUDE_DIRS}" INTERFACE_LINK_LIBRARIES
                                   "${BREAKPAD_LIBRARIES}")
  add_library(breakpad_client INTERFACE IMPORTED GLOBAL)
  set_target_properties(breakpad_client
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_INCLUDE_DIRS}" INTERFACE_LINK_LIBRARIES
                                   "${BREAKPAD_CLIENT_LIBRARIES}")

  add_executable(dump_syms IMPORTED GLOBAL)
  set_property(TARGET dump_syms PROPERTY IMPORTED_LOCATION ${BREAKPAD_DUMPSYM_EXE})

  add_executable(minidump_stackwalk IMPORTED GLOBAL)
  set_property(TARGET minidump_stackwalk PROPERTY IMPORTED_LOCATION ${BREAKPAD_MINIDUMP_STACKWALK_EXE})

  add_executable(sym_upload IMPORTED GLOBAL)
  set_property(TARGET sym_upload PROPERTY IMPORTED_LOCATION ${BREAKPAD_UPLOAD_EXE})
endif()

set(
  PACKAGE_EXPORT
  "BREAKPAD_INCLUDE_DIR;BREAKPAD_INCLUDE_DIRS;BREAKPAD_LIBRARIES;BREAKPAD_CLIENT_LIBRARIES;BREAKPAD_FOUND;BREAKPAD_DUMPSYM_EXE"
  )
