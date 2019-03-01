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

set(TARGET_TAG ${ANDROID_TARGET_TAG})
if(ANDROID_TARGET_TAG MATCHES "windows.*")
  set(TARGET_TAG "windows-x86")
endif()

get_filename_component(PREBUILT_ROOT
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/e2fsprogs/${TARGET_TAG}"
                       ABSOLUTE)

# binplace in bin64
set(DST_DIR "bin64")

if(WINDOWS)
  set(E2FSPROGS_FILES
      cygblkid-1.dll
      cygcom_err-2.dll
      cyge2p-2.dll
      cygext2fs-2.dll
      cyggcc_s-1.dll
      cygiconv-2.dll
      cygintl-8.dll
      cyguuid-1.dll
      cygwin1.dll
      e2fsck.exe
      resize2fs.exe
      tune2fs.exe)
  foreach(EXT_FILE ${E2FSPROGS_FILES})
    set(E2FSPROGS_DEPENDENCIES "${E2FSPROGS_DEPENDENCIES};${PREBUILT_ROOT}/sbin/${EXT_FILE}>${DST_DIR}/${EXT_FILE}")
  endforeach()
else()
  set(E2FSPROGS_FILES e2fsck fsck.ext4 mkfs.ext4 resize2fs tune2fs)
  foreach(EXT_FILE ${E2FSPROGS_FILES})
    set(E2FSPROGS_DEPENDENCIES "${E2FSPROGS_DEPENDENCIES};${PREBUILT_ROOT}/sbin/${EXT_FILE}>${DST_DIR}/${EXT_FILE}")
  endforeach()
endif()

set(E2FSPROGS_FOUND TRUE)
set(PACKAGE_EXPORT "E2FSPROGS_FOUND;E2FSPROGS_DEPENDENCIES")
