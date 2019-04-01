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
get_filename_component(PREBUILT_ROOT
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/mesa/${ANDROID_TARGET_TAG}/lib"
                       ABSOLUTE)

set(MESA_FOUND TRUE)

if(DARWIN_X86_64)
  # No mesa on mac
elseif(LINUX_X86_64)
  set(MESA_DEPENDENCIES
    ${PREBUILT_ROOT}/libGL.so>lib64/gles_mesa/libGL.so;
    ${PREBUILT_ROOT}/libGL.so.1>lib64/gles_mesa/libGL.so.1)
elseif(WINDOWS)
get_filename_component(PREBUILT_ROOT
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/mesa/windows-x86_64/lib"
                       ABSOLUTE)
  set(MESA_DEPENDENCIES "${PREBUILT_ROOT}/opengl32.dll>lib64/gles_mesa/mesa_opengl32.dll")
endif()

set(PACKAGE_EXPORT "MESA_DEPENDENCIES;MESA_FOUND")
