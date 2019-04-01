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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}" ABSOLUTE)

set(FDT_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(FDT_INCLUDE_DIRS "${FDT_INCLUDE_DIR}")
set(FDT_LIBRARIES "${PREBUILT_ROOT}/lib/libfdt${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(FDT_FOUND TRUE)

if(WINDOWS_MSVC_X86_64)
  add_library(FDT STATIC IMPORTED GLOBAL)
  set_target_properties(FDT
                        PROPERTIES IMPORTED_LOCATION "${PREBUILT_ROOT}/lib/libfdt${CMAKE_STATIC_LIBRARY_SUFFIX}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${FDT_INCLUDE_DIRS}")
else()
  if(NOT TARGET FDT)
    add_library(FDT INTERFACE)
    target_link_libraries(FDT INTERFACE "-lfdt")
    target_include_directories(FDT INTERFACE ${FDT_INCLUDE_DIRS})
  endif()
endif()
set(PACKAGE_EXPORT "FDT_INCLUDE_DIR;FDT_INCLUDE_DIRS;FDT_LIBRARIES;FDT_FOUND")
