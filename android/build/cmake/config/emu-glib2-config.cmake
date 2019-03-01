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
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}" ABSOLUTE)

set(GLIB2_INCLUDE_DIR "${PREBUILT_ROOT}/include/glib-2.0;${PREBUILT_ROOT}/lib/glib-2.0/include")
set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIR})

if(DARWIN_X86_64)
  set(GLIB2_LIBRARIES -lglib-2.0 -liconv -lintl "-framework CoreServices" -L${PREBUILT_ROOT}/lib)
elseif(WINDOWS_MSVC_X86_64)
  set(GLIB2_LIBRARIES ${PREBUILT_ROOT}/lib/libglib-2.0.lib ole32::ole32 ws2_32::ws2_32)
elseif(WINDOWS_X86_64)
  set(GLIB2_LIBRARIES -lglib-2.0 ole32::ole32 -L${PREBUILT_ROOT}/lib -lws2_32)
elseif(LINUX_X86_64)
  set(GLIB2_LIBRARIES -lglib-2.0 -lpthread -lrt -L${PREBUILT_ROOT}/lib)
endif()

set(GLIB2_FOUND TRUE)

if(NOT TARGET GLIB2::GLIB2)
  add_library(GLIB2::GLIB2 INTERFACE IMPORTED GLOBAL)
  set_target_properties(GLIB2::GLIB2 PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${GLIB2_INCLUDE_DIRS}"
                        INTERFACE_LINK_LIBRARIES "${GLIB2_LIBRARIES}")

endif()

if(WINDOWS_MSVC_X86_64)
  set(GLIB2_WINDOWS_DEPENDENCIES ${PREBUILT_ROOT}/lib/glib-2-vs11.dll>lib64/glib-2-vs11.dll)
endif()
set(PACKAGE_EXPORT "GLIB2_INCLUDE_DIR;GLIB2_WINDOWS_DEPENDENCIES;GLIB2_INCLUDE_DIRS;GLIB2_LIBRARIES;GLIB2_FOUND;RUNTIME_OS_DEPENDENCIES")
