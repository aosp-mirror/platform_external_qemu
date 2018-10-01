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

get_filename_component(PREBUILT_ROOT "${LOCAL_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/ffmpeg/${LOCAL_TARGET_TAG}" ABSOLUTE)

set(FFMPEG_INCLUDE_DIR "${PREBUILT_ROOT}/include/")
set(FFMPEG_INCLUDE_DIRS "${FFMPEG_INCLUDE_DIRS}")

if (WIN32 OR (${LOCAL_TARGET_TAG} MATCHES ".*windows_msvc.*"))
    set(FFMPEG_LIBAVCODEC "${PREBUILT_ROOT}/lib/libavcodec.lib")
    set(FFMPEG_LIBAVFORMAT "${PREBUILT_ROOT}/lib/libavformat.lib")
    set(FFMPEG_LIBAVUTIL "${PREBUILT_ROOT}/lib/libavutil.lib")
    set(FFMPEG_LIBRARIES "${PREBUILT_ROOT}/lib/libavformat.lib;${PREBUILT_ROOT}/lib/libavfilter.lib;${PREBUILT_ROOT}/lib/libavcodec.lib;${PREBUILT_ROOT}/lib/libswresample.lib;${PREBUILT_ROOT}/lib/libswscale.lib;${PREBUILT_ROOT}/lib/libavutil.lib")
else ()
    set(FFMPEG_LIBAVCODEC "${PREBUILT_ROOT}/lib/libavcodec.a")
    set(FFMPEG_LIBAVFORMAT "${PREBUILT_ROOT}/lib/libavformat.a")
    set(FFMPEG_LIBAVUTIL "${PREBUILT_ROOT}/lib/libavutil.a")
    set(FFMPEG_LIBRARIES "${PREBUILT_ROOT}/lib/libavformat.a;${PREBUILT_ROOT}/lib/libavfilter.a;${PREBUILT_ROOT}/lib/libavcodec.a;${PREBUILT_ROOT}/lib/libswresample.a;${PREBUILT_ROOT}/lib/libswscale.a;${PREBUILT_ROOT}/lib/libavutil.a")
endif ()
set(FFMPEG_FOUND TRUE)
set(PACKAGE_EXPORT "FFMPEG_INCLUDE_DIR;FFMPEG_INCLUDE_DIRS;FFMPEG_LIBRARIES;FFMPEG_CLIENT_LIBRARIES;FFMPEG_FOUND;FFMPEG_LIBAVUTIL;FFMPEG_LIBAVFORMAT;FFMPEG_LIBAVCODEC")


