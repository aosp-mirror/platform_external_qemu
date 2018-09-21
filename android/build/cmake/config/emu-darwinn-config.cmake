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

# For now, this works only on 64 bit linux
if (ANDROID_TARGET_TAG MATCHES linux-x86_64)
get_filename_component(PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/darwinn/${ANDROID_TARGET_TAG}" ABSOLUTE)
set(DARWINN_DEPENDENCIES
    ${PREBUILT_ROOT}/lib/libdarwinn_compiler.so;
    ${PREBUILT_ROOT}/lib/libreference-driver-none.so;
)

set(DARWINN_COMPILE_DEFINITIONS
    "-DDARWINN_COMPILER_TEST_EXTERNAL"
    "-DDARWINN_PORT_ANDROID_EMULATOR=1"
    "-DDARWINN_CHIP_TYPE=USB"
    "-DDARWINN_CHIP_NAME=beagle"
)

android_add_prebuilt_library(DARWINN TESTVECTOR "${PREBUILT_ROOT}/lib/libembedded_compiler_test_data" ${DARWINN_INCLUDE_DIRS} ${DARWINN_COMPILE_DEFINITIONS} "")

set(DARWINN_INCLUDE_DIRS
    ${PREBUILT_ROOT}/include
    ${PREBUILT_ROOT}/include/generated)

set(PACKAGE_EXPORT "DARWINN_DEPENDENCIES;DARWINN_COMPILE_DEFINITIONS;DARWINN_INCLUDE_DIRS")
endif()
