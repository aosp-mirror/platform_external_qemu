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

get_filename_component(PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/curl/${ANDROID_TARGET_TAG}" ABSOLUTE)

set(OPENSSL_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(OPENSSL_INCLUDE_DIRS "${PREBUILT_ROOT}/include")
set(OPENSSL_LIBRARIES "${PREBUILT_ROOT}/lib/libssl${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(OPENSSL_FOUND TRUE)
set(OPENSSL_VERSION "1.0.2j")

set(PACKAGE_EXPORT "OPENSSL_INCLUDE_DIR;OPENSSL_INCLUDE_DIRS;OPENSSL_LIBRARIES;OPENSSL_FOUND;OPENSSL_VERSION")

