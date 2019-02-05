#!/bin/sh

# Copyright 2016 The Android Open Source Project
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
"Build prebuilt protobuf for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option protobuf

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options protobuf
package_builder_parse_package_list

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote protobuf build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        dump "$(builder_text) Building protobuf"

        builder_unpack_package_source googlemock
        builder_unpack_package_source googletest
        builder_unpack_package_source protobuf

        # The original protobuf source archive doesn't come with a 'configure'
        # script, instead it relies on 'autogen.sh' to be called to generate
        # it, but this requires the auto-tools package to be installed on the
        # build machine. This is a problem when performing a remote Darwin
        # build so we provide a patch that completes the source directory
        # with 'configure' and other auto-generated files.
        #
        # Unfortunately, applying the patch uses 'patch' which cannot set
        # the permission bits of the new files to executable, so change them
        # here manually instead.
        (
            PKG_SRC_DIR=$(builder_src_dir)/$(package_list_get_src_dir protobuf)
            cd "$PKG_SRC_DIR"
            chmod a+x configure install-sh
        ) || panic "Could not set 'configure' and 'install-sh' as executables"

        # The prebuilt configuration caueses below problem when
        # host automake version differs from the previous prebuilt ones.  
        # Rebuild the configuration again on the spot for linux systems
        #    .../missing: line 81: aclocal-1.14: command not found
        if [ $(get_build_os) = "linux" ]; then
            (
            PKG_SRC_DIR=$(builder_src_dir)/$(package_list_get_src_dir protobuf)
            cd "$PKG_SRC_DIR"
            ./autogen.sh
            ) || panic "Could not run autogen.sh to re-generate 'configure' and 'install-sh' etc"
        fi

        case $SYSTEM in
            linux*)
                # Passing in $ORIGIN does not work due to multiple variable
                # rewrites, instead we will fix the rpath later, of now
                # we make sure we have enough space for the overwrite
                export CXXFLAGS='-Wl,-rpath=TO_BE_REPLACED/../lib'
                ;;
            *) ;;
        esac

        builder_build_autotools_package protobuf \
                --disable-shared \
                --without-zlib \
                --with-pic \

        copy_directory \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM"

        case $SYSTEM in
            linux*)
                # We need to fixup the rpath to make sure we can find libc++.so
                cp "$(aosp_clang_libcplusplus)" "$INSTALL_DIR/$SYSTEM/lib"
                chrpath  -r \$ORIGIN/../lib  "$INSTALL_DIR/$SYSTEM/bin/protoc"
                ;;
            *) ;;
        esac

    ) || panic "[$SYSTEM] Could not build protobuf!"

done

log "Done building protobuf."
