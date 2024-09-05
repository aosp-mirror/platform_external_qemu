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

if(INCLUDE_NETSIM_CFG_CMAKE)
  return()
endif()
set(INCLUDE_NETSIM_CFG_CMAKE 1)

get_filename_component(
  PREBUILT_ROOT
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/netsim/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

if(WINDOWS_MSVC_X86_64)
  set(NETSIM_DEPENDENCIES
      ${PREBUILT_ROOT}/netsimd.exe>netsimd.exe;
  )

install(PROGRAMS ${PREBUILT_ROOT}/netsimd.exe DESTINATION .)
else()
  set(NETSIM_DEPENDENCIES
      ${PREBUILT_ROOT}/netsimd>netsimd;)

install(PROGRAMS ${PREBUILT_ROOT}/netsimd DESTINATION .)
endif()

set(PACKAGE_EXPORT NETSIM_DEPENDENCIES)
android_license(
  TARGET "NETSIM_DEPENDENCIES"
  LIBNAME "Netsim"
  SPDX "Apache-2.0"
  LICENSE
    "https://android.googlesource.com/platform/tools/netsim/+/refs/heads/main/LICENSE"
  LOCAL ${ANDROID_QEMU2_TOP_DIR}/../../tools/netsim/LICENSE)
