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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}"
  ABSOLUTE)
android_add_prebuilt_library(
  PACKAGE SDL2
  MODULE SDL2 LOCATION "${PREBUILT_ROOT}/lib/libSDL2"
  INCLUDES "${PREBUILT_ROOT}/include/SDL2"
  LIBNAME sdl2
  URL "https://android.googlesource.com/platform/prebuilts/android-emulator-build/archive/+/refs/heads/emu-master-dev/SDL2-2.0.3.tar.gz"
  LICENSE "Zlib"
  NOTICE "https://libsdl.org/license.php"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.SDL2")
