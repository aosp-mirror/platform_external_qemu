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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/automation/aarch64-linux"
  ABSOLUTE)

# binplace in bin64
set(DST_DIR "lib64/compat")

if(LINUX_AARCH64)
  set(AARCH64LINUXGCCLIB_FILES gcclib.tgz)
  foreach(EXT_FILE ${AARCH64LINUXGCCLIB_FILES})
    set(AARCH64COMPAT_DEPENDENCIES
        "${AARCH64COMPAT_DEPENDENCIES};${PREBUILT_ROOT}/${EXT_FILE}>${DST_DIR}/${EXT_FILE}"
    )
  endforeach()

endif()


get_filename_component(
  SCRIPTROOT
  "${ANDROID_QEMU2_TOP_DIR}/android/scripts/aarch64-linux/launcher"
  ABSOLUTE)

# binplace in bin64
set(DST_DIR ".")

if(LINUX_AARCH64)
  set(AARCH64LINUXPOSTINSTALL_FILES postemuinstall.sh)
  foreach(EXT_FILE ${AARCH64LINUXPOSTINSTALL_FILES})
    set(AARCH64COMPAT_DEPENDENCIES
        "${AARCH64COMPAT_DEPENDENCIES};${SCRIPTROOT}/${EXT_FILE}>${DST_DIR}/${EXT_FILE}"
    )
  endforeach()

  android_license(
    TARGET AARCH64COMPAT_DEPENDENCIES
    LIBNAME e2fs
    EXECUTABLES ${AARCH64LINUXPOSTINSTALL_FILES}
    URL "https://android.googlesource.com/platform/prebuilts/android-emulator-build/archive/+/refs/heads/emu-master-dev/e2fsprogs-1.42.13-patches.tar.xz"
    SPDX "GPL-2.0-only"
    LICENSE "http://www.linfo.org/e2fsprogs.html"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.E2FS")
endif()

set(AARCH64COMPAT_FOUND TRUE)
set(PACKAGE_EXPORT "AARCH64COMPAT_FOUND;AARCH64COMPAT_DEPENDENCIES")
