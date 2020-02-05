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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/msvc/"
  ABSOLUTE)
set(MSVC_FOUND TRUE)
set(MSVC_DEPENDENCIES
    "${PREBUILT_ROOT}/msvcp140.dll>./msvcp140.dll"
    "${PREBUILT_ROOT}/vcruntime140.dll>./vcruntime140.dll"
    "${PREBUILT_ROOT}/msvcp140.dll>lib64/msvcp140.dll"
    "${PREBUILT_ROOT}/vcruntime140.dll>lib64/vcruntime140.dll")
set(PACKAGE_EXPORT "MSVC_DEPENDENCIES;MSVC_FOUND")
android_license(
  TARGET MSVC_DEPENDENCIES
  LIBNAME vcredist
  EXECUTABLES msvcp140.dll vcruntime140.dll msvcp140.dll vcruntime140.dll
  URL "https://docs.microsoft.com/en-us/visualstudio/productinfo/2017-redistribution-vs"
  SPDX "2017-redistribution-vs"
  LICENSE
    "https://marketplace.visualstudio.com/items/VisualStudioProductTeam.VisualStudioProjectSystemExtensibilityPreview/license"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.VSREDIST")
