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
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/curl/${ANDROID_TARGET_TAG}"
                       ABSOLUTE)

set(OPENSSL_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(OPENSSL_INCLUDE_DIRS "${PREBUILT_ROOT}/include")
set(
  OPENSSL_LIBRARIES
  "${PREBUILT_ROOT}/lib/libssl${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
set(OPENSSL_FOUND TRUE)
set(OPENSSL_VERSION "1.0.2j")

if(ANDROID_TARGET_TAG MATCHES "windows.*" AND NOT TARGET crypto)
  prebuilt(GLIB2)
  add_library(crypto INTERFACE IMPORTED GLOBAL)
  set_target_properties(
    crypto
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIRS}" INTERFACE_LINK_LIBRARIES
               "${PREBUILT_ROOT}/lib/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX};ws2_32::ws2_32;GLIB2::GLIB2")
else()
  add_library(crypto STATIC IMPORTED GLOBAL)
  set_target_properties(crypto
                        PROPERTIES IMPORTED_LOCATION "${PREBUILT_ROOT}/lib/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${PREBUILT_ROOT}/include")

endif()

if(NOT TARGET ssl)
  add_library(ssl STATIC IMPORTED GLOBAL)
  set_target_properties(ssl
                        PROPERTIES IMPORTED_LOCATION
                                   "${PREBUILT_ROOT}/lib/libssl${CMAKE_STATIC_LIBRARY_SUFFIX}"
                                   INTERFACE_LINK_LIBRARIES
                                   "crypto"
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   "${PREBUILT_ROOT}/include")
endif()

set(PACKAGE_EXPORT "OPENSSL_INCLUDE_DIR;OPENSSL_INCLUDE_DIRS;OPENSSL_LIBRARIES;OPENSSL_FOUND;OPENSSL_VERSION")
