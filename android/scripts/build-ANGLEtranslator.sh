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
"Build prebuilt ANGLE shader translator for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/ANGLEtranslator"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options ANGLEtranslator

BUILD_SRC_DIR=$TEMP_DIR/angle

# Unpack package source into $BUILD_SRC_DIR if needed.
# $1: Package basename.
copy_angle_source () {
    local PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_SRC_TIMESTAMP=$PKG_BUILD_DIR/timestamp-angle
    if [ ! -f "$PKG_SRC_TIMESTAMP" ]; then
        copy_directory $AOSP_DIR/external/angle $PKG_BUILD_DIR
        touch $PKG_SRC_TIMESTAMP
    fi
}

# $1: Package basename (e.g. 'libpthread-stubs-0.3')
# $2+: Extra configuration options.
build_angle_libraries () {
    local PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/angle
    copy_angle_source
    PKG_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/angle-timestamp
    if [ ! -f "$PKG_TIMESTAMP" -o -n "$OPT_FORCE" ]; then
        case $SYSTEM in
            darwin*)
                # Required for proper build on Darwin!
                builder_disable_verbose_install
                ;;
        esac
        builder_build_autotools_package_full_install \
            "$PKG_BUILD_DIR" \
            "$PKG_BUILD_DIR" \
            "" \
            "$@"

        # We don't have a make install command at all since the Makefile is
        # auto-generated, so we just copy the files we want directly.
        copy_directory_files \
                "$PKG_BUILD_DIR/out/Debug/obj.target/src/" \
                "$(builder_install_prefix)/lib" \
                libangle_common.a \
                libpreprocessor.a \
                libtranslator_lib.a \
                libtranslator_static.a

        copy_directory \
                "$PKG_BUILD_DIR/include/GLSLANG" \
                "$(builder_install_prefix)/include/GLSLANG"

        touch "$PKG_TIMESTAMP"
    fi
}

# Perform a Darwin build through ssh to a remote machine.
# $1: Darwin host name.
# $2: List of darwin target systems to build for.
do_remote_darwin_build () {
    builder_prepare_remote_darwin_build \
            "/tmp/$USER-rebuild-darwin-ssh-$$/ANGLEtranslator-build"

    copy_directory "$ARCHIVE_DIR" "$DARWIN_PKG_DIR"/archive

    local PKG_DIR="$DARWIN_PKG_DIR"
    local REMOTE_DIR=/tmp/$DARWIN_PKG_NAME
    # Generate a script to rebuild all binaries from sources.
    # Note that the use of the '-l' flag is important to ensure
    # that this is run under a login shell. This ensures that
    # ~/.bash_profile is sourced before running the script, which
    # puts MacPorts' /opt/local/bin in the PATH properly.
    #
    # If not, the build is likely to fail with a cryptic error message
    # like "readlink: illegal option -- f"
    cat > $PKG_DIR/build.sh <<EOF
#!/bin/bash -l
PROGDIR=\$(dirname \$0)
\$PROGDIR/scripts/$(program_name) \\
    --build-dir=$REMOTE_DIR/build \\
    --host=$(spaces_to_commas "$DARWIN_SYSTEMS") \\
    --install-dir=$REMOTE_DIR/install-prefix \\
    --prebuilts-dir=$REMOTE_DIR \\
    --aosp-dir=$REMOTE_DIR/aosp \\
    $DARWIN_BUILD_FLAGS
EOF
    builder_run_remote_darwin_build

    run rm -rf "$PKG_DIR"

    run mkdir -p "$INSTALL_DIR" ||
            panic "Could not create final directory: $INSTALL_DIR"

    for SYSTEM in $DARWIN_SYSTEMS; do
        dump "[$SYSTEM] Retrieving remote darwin binaries"
        run rm -rf "$INSTALL_DIR"/* &&
        run $ANDROID_EMULATOR_SSH_WRAPPER rsync -haz --delete \
                --exclude=intermediates --exclude=libs \
                $DARWIN_SSH:$REMOTE_DIR/install-prefix/$SYSTEM \
                $INSTALL_DIR
    done
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote ANGLE translator libraries build for: $DARWIN_SYSTEMS"
    do_remote_darwin_build "$DARWIN_SSH" "$DARWIN_SYSTEMS"
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        build_angle_libraries

        # Copy binaries necessary for the build itself as well as static libraries.
        copy_directory \
                "$(builder_install_prefix)/" \
                "$INSTALL_DIR/$SYSTEM/"

        dump "[$SYSTEM] Done building ANGLE translator libraries"

    ) || panic "[$SYSTEM] Could not build ANGLE translator libraries!"

done

log "Done building ANGLE translator libraries."
