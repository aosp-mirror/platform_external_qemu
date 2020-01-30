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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/virglrenderer/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

android_add_interface(
  TARGET virglrenderer-headers
  LIBNAME
    virglrenderer
    URL
    "https://android.googlesource.com/platform/external/virglrenderer/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../virglrenderer"
  NOTICE "REPO/NOTICE"
  LICENSE "MIT")

set(VIRGLRENDERER_INCLUDE_DIR "${PREBUILT_ROOT}/include/virgl")
set(VIRGLRENDERER_INCLUDE_DIRS "${VIRGLRENDERER_INCLUDE_DIR}")
set(VIRGLRENDERER_FOUND TRUE)
set(PACKAGE_EXPORT
    "VIRGLRENDERER_INCLUDE_DIR;VIRGLRENDERER_INCLUDE_DIRS;VIRGLRENDERER_FOUND")

target_include_directories(virglrenderer-headers
                           INTERFACE "${VIRGLRENDERER_INCLUDE_DIRS}")
