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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/mesa/${ANDROID_TARGET_TAG}/lib"
  ABSOLUTE)

set(MESA_FOUND TRUE)

if(DARWIN_X86_64 OR DARWIN_AARCH64)
  # No mesa on mac
  android_license(TARGET "MESA_DEPENDENCIES" LIBNAME None SPDX None LICENSE None
                  LOCAL None)
elseif(LINUX_AARCH64)
  android_license(TARGET "MESA_DEPENDENCIES" LIBNAME None SPDX None LICENSE None
                  LOCAL None)
elseif(LINUX_X86_64)
  set(MESA_DEPENDENCIES ${PREBUILT_ROOT}/libGL.so>lib64/gles_mesa/libGL.so;
                        ${PREBUILT_ROOT}/libGL.so.1>lib64/gles_mesa/libGL.so.1)
  android_license(
    TARGET MESA_DEPENDENCIES
    LIBNAME mesa
    EXECUTABLES libGL.so libGL.so.1
    URL "https://android.googlesource.com/platform/prebuilts/android-emulator-build/archive/+/refs/heads/emu-master-dev/MesaLib-10.4.2.tar.bz2"
    SPDX "MIT"
    LICENSE "https://mesa3d.org/license.html"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.MESA")
elseif(WINDOWS)
  get_filename_component(
    PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/mesa/windows-x86_64/lib"
    ABSOLUTE)
  set(MESA_DEPENDENCIES
      "${PREBUILT_ROOT}/opengl32.dll>lib64/gles_mesa/mesa_opengl32.dll")
  android_license(
    TARGET MESA_DEPENDENCIES
    LIBNAME mesa
    EXECUTABLES opengl32.dll mesa_opengl32.dll
    URL "https://android.googlesource.com/platform/prebuilts/android-emulator-build/archive/+/refs/heads/emu-master-dev/MesaLib-10.4.2.tar.bz2"
    SPDX "MIT"
    LICENSE "https://mesa3d.org/license.html"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.MESA")
endif()

set(PACKAGE_EXPORT "MESA_DEPENDENCIES;MESA_FOUND")

android_license("MESA" "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.GPLv2")
