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
"Build prebuilt ANGLE for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/ANGLE"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options ANGLE

BUILD_SRC_DIR=$TEMP_DIR/angle

# Build the ANGLE packages.
# Based on builder_build_autotools_package_full_install,
# which we should switch to if possible.
#
# $1: Source directory.
# $2+: Additional configure options.
build_angle_package () {
    local PKG_FULLNAME="$(basename "$1")"
    dump "$(builder_text) Building $PKG_FULLNAME"

    local PKG_BUILD_DIR="$1"
    shift

    local SED_FLAGS="-i"
    case $SYSTEM in
        darwin*)
            SED_FLAGS=-e
            ;;
    esac

    run mkdir -p "$PKG_BUILD_DIR" &&
    run cd "$PKG_BUILD_DIR" &&
    export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
    export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
    export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
    export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&

    # The Makefile and *.mk files required to build ANGLE are actually synced via
    # gclient and not included in the repositry. This script ensures that they
    # are properly synced and setup.

    # Ensure the gclient command is available.
    # TODO: we should pick a specific commit to pull just in case.
    run git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git &&
    export PATH=`pwd`/depot_tools:"$PATH" &&

    # Ensure ninja, included in the depot tools, is used to generator the build
    # files, since it is a cross-platform solution.
    export GYP_GENERATORS=ninja &&

    # Create gclient file and sync the build files
    run python scripts/bootstrap.py &&

    # TODO: gclient sync pulls different files per OS - we probably need to pull
    # Windows-specific files for cross compilation of Windows static libraries on
    # Linux.
    run gclient sync &&

    # HACK: The .mk files are created with a command to make the static libaries
    # just a stripped listing of .o files instead of full libraries (most likely
    # for size reasons). We want full libraries since we'll be copying them
    # somewhere else.
    find ./out/Debug/obj/ -type f -name "*.ninja" -exec sed $SED_FLAGS 's/alink_thin/alink/g' {} + &&

    # ninja is provided for each platform in the previously pulled depot_tools
    run ninja -C out/Debug
}

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
        build_angle_package \
            "$PKG_BUILD_DIR" \
            "" \
            "$@"

        # We don't have a make install command at all since the Makefile is
        # auto-generated, so we just copy the files we want directly.
        case $SYSTEM in
            linux*)
                copy_directory_files \
                    "$PKG_BUILD_DIR/out/Debug/obj/src" \
                    "$(builder_install_prefix)/lib" \
                    libangle_common.a \
                    libpreprocessor.a \
                    libtranslator_lib.a \
                    libtranslator_static.a
                ;;
            darwin*)
                copy_directory_files \
                    "$PKG_BUILD_DIR/out/Debug/" \
                    "$(builder_install_prefix)/lib" \
                    libangle_common.a \
                    libpreprocessor.a \
                    libtranslator_lib.a \
                    libtranslator_static.a
                ;;
        esac

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
            "/tmp/$USER-rebuild-darwin-ssh-$$/ANGLE-build"

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
    dump "Remote ANGLE libraries build for: $DARWIN_SYSTEMS"
    do_remote_darwin_build "$DARWIN_SSH" "$DARWIN_SYSTEMS"
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        case $SYSTEM in
            linux*)
                builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

                build_angle_libraries

                # Copy binaries necessary for the build itself as well as static libraries.
                copy_directory \
                        "$(builder_install_prefix)/" \
                        "$INSTALL_DIR/$SYSTEM/"

                dump "[$SYSTEM] Done building ANGLE libraries"
                ;;
            darwin*)
                builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

                build_angle_libraries

                # Copy binaries necessary for the build itself as well as static libraries.
                copy_directory \
                        "$(builder_install_prefix)/" \
                        "$INSTALL_DIR/$SYSTEM/"

                dump "[$SYSTEM] Done building ANGLE libraries"
                ;;
        esac

    ) || panic "[$SYSTEM] Could not build ANGLE libraries!"

done

log "Done building ANGLE libraries."
