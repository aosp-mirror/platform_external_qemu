# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

get_filename_component(
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/ANGLE/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

set(ANGLE_INCLUDE_DIRS "${PREBUILT_ROOT}/include")
set(ANGLE_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(ANGLE_LIBRARIES "${PREBUILT_ROOT}/lib/libtranslator_static${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libtranslator_lib${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libpreprocessor${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libangle_common${CMAKE_STATIC_LIBRARY_SUFFIX}")

if(NOT TARGET ANGLE::ANGLE)
    add_library(ANGLE::ANGLE INTERFACE IMPORTED GLOBAL)
    set_target_properties(ANGLE::ANGLE PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${ANGLE_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${ANGLE_LIBRARIES}"
    )
endif()

set(ANGLE_FOUND TRUE)
set(PACKAGE_EXPORT "ANGLE_INCLUDE_DIR;ANGLE_INCLUDE_DIRS;ANGLE_LIBRARIES;ANGLE_FOUND")
