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
#  android/scripts/build-ffmpeg.sh --host=linux-x86 --verbose --force
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
            MY_FLAGS="--extra-ldflags=\"-ldl\""
            ;;
        linux-x86)
            MY_FLAGS="--target-os=linux --arch=x86 --enable-cross-compile --cc=gcc --extra-ldflags=\"-ldl\""
            ;;
        windows-x86)
            MY_FLAGS="--target-os=mingw32 --arch=x86 --enable-cross-compile --cc=gcc"
            ;;
        windows-x86_64)
            MY_FLAGS="--target-os=mingw32 --arch=x86_64 --enable-cross-compile --cc=gcc "
            ;;
        darwin-*)
            # Use host compiler.
            MY_FLAGS=
            ;;
        *)
            panic "Host system '$CURRENT_HOST' is not supported by this script!"
            ;;
        esac

        builder_build_autotools_package_ffmpeg ffmpeg \
                $MY_FLAGS \
                --extra-cflags=\"-I$PREBUILTS_DIR/common/x264/$SYSTEM/include\" \
                --extra-ldflags=\"-L$PREBUILTS_DIR/common/x264/$SYSTEM/lib\" \
                --enable-static \
                --disable-doc \
                --enable-gpl \
                --enable-asm \
                --enable-yasm \
                --disable-avdevice \
                --enable-avresample \
                --enable-libx264 \
                --disable-protocol=tls \
                --disable-protocol=tls_securetransport \
                --disable-openssl \
                --disble-iconv

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
