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

# you need to install yasm
# on Linux: sudo apt-get install yams
# on MAC:
# (1) ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
# (2) brew install yams

. $(dirname "$0")/utils/common.shi

shell_import utils/aosp_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/install_dir.shi
shell_import utils/option_parser.shi
shell_import utils/package_list_parser.shi
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt x264 for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/x264"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options x264
package_builder_parse_package_list

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote x264 build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        dump "$(builder_text) Building x264"

        builder_unpack_package_source x264
        case $SYSTEM in
          linux-x86_64)
                MY_FLAGS=" --enable-pic \
                    --extra-cflags=-fPIC \
                    --disable-asm \
                "
                ;;
          linux-aarch64)
                MY_FLAGS=" --enable-pic \
                    --extra-cflags=-fPIC \
                "
                ;;
          windows_msvc-x86_64)
                MY_FLAGS="--host=x86_64-pc-mingw64 --extra-cflags=-ffast-math --disable-asm"
                warn "X264 encoder is disabled until b/150910477 is fixed."
                export CC=cl
                export RC=rc /FO x264res.o f || true
                _SHU_BUILDER_GNU_CONFIG_HOST_FLAG=
                ;;
          darwin-x86_64)
                # Use host compiler.
                MY_FLAGS=" --enable-pic \
                    --extra-cflags=-fPIC \
                    --disable-asm \
                "
                ;;
          darwin-aarch64)
                # Use host compiler.
                MY_FLAGS=" --enable-pic \
                    --extra-cflags=-fPIC \
                    --host=aarch64-apple-darwin20.0.0 \
                "
                ;;
          *)
                panic "Host system '$CURRENT_HOST' is not supported by this script!"
                ;;
        esac

        builder_build_autotools_package x264 \
                $MY_FLAGS \
                --disable-cli  \
                --enable-static

        # Copy binaries necessary for the build itself as well as static
        # libraries. (Note missing files are nops.)
        copy_directory_files \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM" \
                lib/libx264.a

        copy_directory_files \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM" \
                lib/libx264.lib

        copy_directory \
                "$(builder_install_prefix)/include" \
                "$INSTALL_DIR/$SYSTEM/include"

    ) || panic "[$SYSTEM] Could not build x264!"

done

log "Done building x264."
