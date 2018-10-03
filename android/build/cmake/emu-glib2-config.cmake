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

get_filename_component(PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}" ABSOLUTE)

set(GLIB2_INCLUDE_DIR "${PREBUILT_ROOT}/include/glib-2.0;${PREBUILT_ROOT}/lib/glib-2.0/include")
set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIR})

if(APPLE OR ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
    set(GLIB2_LIBRARIES -liconv -lintl)
elseif (WIN32 OR ANDROID_TARGET_TAG MATCHES "windows.*")
    set(GLIB2_LIBRARIES -lole32)
elseif (UNIX OR ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
    set(GLIB2_LIBRARIES -lpthread -lrt)
endif()


set(GLIB2_FOUND TRUE)

set(PACKAGE_EXPORT "GLIB2_INCLUDE_DIR;GLIB2_INCLUDE_DIRS;GLIB2_LIBRARIES;GLIB2_FOUND")


