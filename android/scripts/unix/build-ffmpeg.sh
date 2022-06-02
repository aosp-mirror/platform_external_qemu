#!/bin/sh

# Copyright 2015 The Android Open Source Project
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

# how to use:
#  android/scripts/build-ffmpeg.sh --host=linux-x86_64 --verbose --force
#  android/scripts/build-ffmpeg.sh --host=windows-x86_64 --verbose --force
#
# yasm is required to build:
# Download yasm source code from http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz
# Unpack tar xvzf yasm-1.2.0.tar.gz
# cd yasm-1.2.0
# Configure and build:
#
# ./configure && make -j 4 && sudo make install
#

. $(dirname "$0")/utils/common.shi

# set -x
shell_import utils/aosp_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/install_dir.shi
shell_import utils/option_parser.shi
shell_import utils/package_list_parser.shi
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt ffmpeg for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/ffmpeg"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options ffmpeg
package_builder_parse_package_list


if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote ffmpeg build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi


for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        dump "$(builder_text) Building ffmpeg"

        builder_unpack_package_source ffmpeg

        case $SYSTEM in
          linux-x86_64)
            MY_FLAGS="--extra-ldflags=\"-ldl\" \
              --disable-xlib \
              --extra-ldflags=\"-L$PREBUILTS_DIR/common/x264/$SYSTEM/lib\" \
              --enable-pic \
              --extra-ldflags=\"-L$PREBUILTS_DIR/common/libvpx/$SYSTEM/lib\" \
              "
            ;;
        linux-aarch64)
          MY_FLAGS="--extra-ldflags=\"-ldl\" \
            --disable-xlib \
            --extra-ldflags=\"-L$PREBUILTS_DIR/common/x264/$SYSTEM/lib\" \
            --enable-pic \
            --enable-cross-compile --arch=aarch64 --target-os=linux \
            --extra-ldflags=\"-L$PREBUILTS_DIR/common/libvpx/$SYSTEM/lib\ \
            "
            ;;
        windows_msvc-x86_64)
          MY_FLAGS="--target-os=win64 \
            --arch=x86_64 \
            --toolchain=msvc  \
            --extra-cflags=\"-Wno-everything\" \
            --extra_cxxflags=\"-Wno-everything\" \
            --enable-cross-compile \
            --extra-ldflags=\"-Wl,-libpath:$PREBUILTS_DIR/common/x264/windows_msvc-x86_64/lib\"  \
            --extra-ldflags=\"-Wl,-libpath:$PREBUILTS_DIR/common/libvpx/windows_msvc-x86_64/lib\""
            ;;
        darwin-*)
            # Use host compiler.
            MY_FLAGS="--disable-iconv  \
              --extra-ldflags=\"-L$PREBUILTS_DIR/common/x264/$SYSTEM/lib\" \
              --enable-pic \
              --extra-ldflags=\"-L$PREBUILTS_DIR/common/libvpx/$SYSTEM/lib\" \
              "
            ;;
        *)
            panic "Host system '$CURRENT_HOST' is not supported by this script!"
            ;;
        esac


        # Be very careful with changing the prebuilts:
        #
        # - Only enable muxers you need.
        # - Only enable de-muxets you need
        # - Only enable protocols that you need
        # - Only enable coders you need
        # - Only enable decoders you need
        #
        # Ffmpeg is basically a self contained enormous static library with all
        # the filters, codec, protocols and (de)muxers you would ever need.
        # Due to this you get *EVERYTHING* if you link against it, if you
        # never use speex, or rtmp.
        #
        #  - webm: used by recording, we need to be able to read/write
        #  - gif: animated gifs, used by recording
        #  - matroska: used by recording, we need this for opening files we
        #  created
        #  - mov/mp4: used by offworld (automated testing)
        #  wrote.
        # - vorbis: audio codec for recording
        # - gif: video codec used for recording
        # - vp9: video coded used for recording
        # - h264: encoder used by newer system images.
        builder_build_autotools_package_ffmpeg ffmpeg \
                $MY_FLAGS \
                --enable-libx264 \
                --extra-cflags=-O3 \
                --extra-cflags=\"-I$PREBUILTS_DIR/common/x264/$SYSTEM/include\" \
                --extra-cflags=\"-I$PREBUILTS_DIR/common/libvpx/$SYSTEM/include\" \
                --enable-static \
                --disable-doc \
                --disable-programs \
                --enable-gpl \
                --disable-asm \
                --disable-yasm \
                --disable-avdevice \
                --disable-filters \
                --enable-avresample \
                --enable-libvpx \
                --disable-muxers \
                --enable-muxer=gif \
                --enable-muxer=webm \
                --enable-muxer=webm_chunk_muxer \
                --enable-muxer=webm_dash_manifest_muxer \
                --disable-demuxers \
                --enable-demuxer=webm \
                --enable-demuxer=matroska \
                --enable-demuxer=webm_chunk_muxer \
                --enable-demuxer=mov \
                --enable-demuxer=webm_dash_manifest_muxer \
                --disable-protocols \
                --enable-protocol=file \
                --disable-encoders \
                --enable-encoder=gif  \
                --enable-encoder=libvpx_vp9 \
                --enable-encoder=vorbis \
                --disable-decoders \
                --enable-decoder=gif  \
                --enable-decoder=libvpx_vp9 \
                --enable-decoder=vorbis \
                --enable-decoder=hevc \
                --enable-decoder=h264

        # Copy binaries necessary for the build itself as well as static
        # libraries.
        copy_directory \
                "$(builder_install_prefix)/lib" \
                "$INSTALL_DIR/$SYSTEM/lib"

        copy_directory \
                "$(builder_install_prefix)/include" \
                "$INSTALL_DIR/$SYSTEM/include"

    ) || panic "[$SYSTEM] Could not build ffmpeg!"

done

log "Done building ffmpeg."
