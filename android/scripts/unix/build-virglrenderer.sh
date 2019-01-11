#!/bin/sh

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

. $(dirname "$0")/utils/common.shi

shell_import utils/aosp_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/install_dir.shi
shell_import utils/option_parser.shi
shell_import utils/package_list_parser.shi
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt virglrenderer for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/virglrenderer"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options virglrenderer
package_builder_parse_package_list

PKG_SRC_DIR=$TEMP_DIR/build/virglrenderer

prepare_source_tree() {
  dump "Preparing source tree"
  copy_directory "$AOSP_DIR/external/virglrenderer" "$PKG_SRC_DIR"
  NOCONFIGURE=1 run $PKG_SRC_DIR/autogen.sh
}

install_simple_package() {
  dump "$(builder_text) Building virglrenderer"
  local PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/virglrenderer
  mkdir -p $PKG_BUILD_DIR
  cd $PKG_BUILD_DIR && EPOXY_CFLAGS="-I $AOSP_DIR/prebuilts/android-emulator-build/common"  EPOXY_LIBS="-L." run $PKG_SRC_DIR/configure --prefix=$(builder_install_prefix) --disable-egl --disable-shared --enable-static $(builder_gnu_config_host_flag) && cd -
  run make -C $PKG_BUILD_DIR/src -j$NUM_JOBS install
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote virglrenderer build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi

prepare_source_tree

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        install_simple_package

        copy_directory_files \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM" \
                lib/libvirglrenderer.a

        copy_directory \
                "$(builder_install_prefix)/include/" \
                "$INSTALL_DIR/$SYSTEM/include/"

    ) || panic "[$SYSTEM] Could not build virglrenderer!"

done

log "Done building virglrenderer."
