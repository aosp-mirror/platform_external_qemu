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

. $(dirname "$0")/utils/common.shi

shell_import utils/aosp_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/install_dir.shi
shell_import utils/option_parser.shi
shell_import utils/package_list_parser.shi
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt libvpx for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/libvpx"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options libvpx
package_builder_parse_package_list

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote libvpx build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        dump "$(builder_text) Building libvpx"

        builder_unpack_package_source libvpx

        case $SYSTEM in
        linux-x86_64)
            MY_FLAGS="--target=x86_64-linux-gcc"
            ;;
        linux-aarch64)
            MY_FLAGS="--target=arm64-linux-gcc"
            ;;
        windows-x86_64)
            # We cannot strip symbols in x86_64
            MY_FLAGS="--target=x86_64-win64-gcc --enable-debug"
            ;;
        darwin-*)
            # Use host compiler.
            MY_FLAGS="--target=x86_64-darwin10-gcc"
            ;;
        *)
            panic "Host system '$CURRENT_HOST' is not supported by this script!"
            ;;
        esac

        builder_build_autotools_package_ffmpeg libvpx \
                $MY_FLAGS \
                --enable-pic \
                --enable-static \
                --enable-vp9-highbitdepth \
                --disable-tools \
                --disable-unit-tests \
                --extra-cxxflags="-Doff_t=__off64_t -fPIC"

        # Copy binaries necessary for the build itself as well as static
        # libraries.
        copy_directory_files \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM" \
                lib/libvpx.a

        copy_directory \
                "$(builder_install_prefix)/include" \
                "$INSTALL_DIR/$SYSTEM/include"

    ) || panic "[$SYSTEM] Could not build libvpx!"

done

log "Done building libvpx."
