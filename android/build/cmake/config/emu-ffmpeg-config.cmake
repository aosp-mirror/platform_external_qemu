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
if (NOT WINDOWS_MSVC_X86_64)
  prebuilt(X264)
endif()
prebuilt(VPX)
get_filename_component(
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/ffmpeg/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

set(FFMPEG_INCLUDE_DIR "${PREBUILT_ROOT}/include/")
set(FFMPEG_INCLUDE_DIRS "${FFMPEG_INCLUDE_DIR}")
set(FFMPEG_LIBAVCODEC "${PREBUILT_ROOT}/lib/libavcodec${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(FFMPEG_LIBAVFORMAT "${PREBUILT_ROOT}/lib/libavformat${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(FFMPEG_LIBAVUTIL "${PREBUILT_ROOT}/lib/libavutil${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(FFMPEG_LIBRARIES "${PREBUILT_ROOT}/lib/libavformat${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libavfilter${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libavcodec${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libswresample${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libswscale${CMAKE_STATIC_LIBRARY_SUFFIX};${PREBUILT_ROOT}/lib/libavutil${CMAKE_STATIC_LIBRARY_SUFFIX};${X264_LIBRARIES};${VPX_LIBRARIES}")
if(WINDOWS)
  # We need winsock for avformat, so make that dependency explicit
  list(APPEND FFMPEG_LIBRARIES ws2_32::ws2_32 secur32::secur32)
endif()
if(DARWIN_X86_64)
  # Well, macos needs something extra as well
  list(APPEND FFMPEG_LIBRARIES "-framework VideoToolbox")
  list(APPEND FFMPEG_LIBRARIES "-framework VideoDecodeAcceleration")
  list(APPEND FFMPEG_LIBRARIES "-framework AudioToolbox")
  list(APPEND FFMPEG_LIBRARIES "-lbz2")
endif()
set(FFMPEG_FOUND TRUE)

if(NOT TARGET FFMPEG::FFMPEG)
    add_library(FFMPEG::FFMPEG INTERFACE IMPORTED GLOBAL)
    set_target_properties(FFMPEG::FFMPEG PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}"
    )
endif()

set(
  PACKAGE_EXPORT
  "FFMPEG_INCLUDE_DIR;FFMPEG_INCLUDE_DIRS;FFMPEG_LIBRARIES;FFMPEG_CLIENT_LIBRARIES;FFMPEG_FOUND;FFMPEG_LIBAVUTIL;FFMPEG_LIBAVFORMAT;FFMPEG_LIBAVCODEC"
  )
