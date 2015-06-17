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
"Build prebuilt Qt host libraries."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option qt

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

ARCHIVE_DIR=$PREBUILTS_DIR/archive
if [ ! -d "$ARCHIVE_DIR" ]; then
    dump "Downloading dependencies sources first."
    $(program_directory)/download-sources.sh \
        --verbosity=$(get_verbosity) \
        --prebuilts-dir="$PREBUILTS_DIR" ||
            panic "Could not download source archives!"
fi
if [ ! -d "$ARCHIVE_DIR" ]; then
    panic "Missing archive directory: $ARCHIVE_DIR"
fi
PACKAGE_LIST=$ARCHIVE_DIR/PACKAGES.TXT
if [ ! -f "$PACKAGE_LIST" ]; then
    panic "Missing package list file, run download-sources.sh: $PACKAGE_LIST"
fi

package_builder_process_options qt

package_list_parse_file "$PACKAGE_LIST"

BUILD_SRC_DIR=$TEMP_DIR/src

# Unpack package source into $BUILD_SRC_DIR if needed.
# $1: Package basename.
unpack_package_source () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_NAME=$(package_list_get_src_dir $1)
    PKG_SRC_TIMESTAMP=$BUILD_SRC_DIR/timestamp-$PKG_NAME
    if [ ! -f "$PKG_SRC_TIMESTAMP" ]; then
        package_list_unpack_and_patch "$1" "$ARCHIVE_DIR" "$BUILD_SRC_DIR"
        touch $PKG_SRC_TIMESTAMP
    fi
}

# $1: Package name (e.g. 'qt-base')
# $2+: Extra configuration options.
build_qt_package () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_NAME=$(package_list_get_src_dir $1)
    unpack_package_source "$1"
    shift
    PKG_SRC_DIR="$BUILD_SRC_DIR/$PKG_NAME"
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/$PKG_NAME
    PKG_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/$PKG_NAME-timestamp
    if [ ! -f "$PKG_TIMESTAMP" -o -n "$OPT_FORCE" ]; then
        (
            run mkdir -p "$PKG_BUILD_DIR" &&
            run cd "$PKG_BUILD_DIR" &&
            export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
            export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
            export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
            export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&
            run "$PKG_SRC_DIR"/configure \
                -prefix $_SHU_BUILDER_PREFIX \
                -release \
                -opensource \
                -confirm-license \
                "$@" &&
            run make -j$NUM_JOBS V=1 &&
            run make install -j$NUM_JOBS V=1
        ) ||
        panic "Could not build and install $1"

        touch "$PKG_TIMESTAMP"
    fi
}

# Perform a Darwin build through ssh to a remote machine.
# $1: Darwin host name.
# $2: List of darwin target systems to build for.
do_remote_darwin_build () {
    builder_prepare_remote_darwin_build \
            "/tmp/$USER-rebuild-darwin-ssh-$$/qt-build"

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

    local BINARY_DIR=$INSTALL_DIR
    run mkdir -p "$BINARY_DIR" ||
            panic "Could not create final directory: $BINARY_DIR"

    for SYSTEM in $DARWIN_SYSTEMS; do
        dump "[$SYSTEM] Retrieving remote darwin binaries"
        run scp -r "$DARWIN_SSH":$REMOTE_DIR/install-prefix/$SYSTEM \
                $BINARY_DIR/

        timestamp_set "$INSTALL_DIR/$SYSTEM" google-breakpad
    done
}

if [ -z "$OPT_FORCE" ]; then
    builder_check_all_timestamps "$INSTALL_DIR" google-breakpad
fi

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote Qt build for: $DARWIN_SYSTEMS"
    do_remote_darwin_build "$DARWIN_SSH" "$DARWIN_SYSTEMS"
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host "$SYSTEM" "$AOSP_DIR" "$INSTALL_DIR/$SYSTEM"

        EXTRA_CONFIGURE_FLAGS=
        var_append EXTRA_CONFIGURE_FLAGS \
                -no-c++11 \
                -no-rpath \
                -no-gtkstyle \
                -v

        case $SYSTEM in
            linux*)
                var_append EXTRA_CONFIGURE_FLAGS -qt-xcb
                ;;
            windows*)
                case $SYSTEM in
                    windows-x86)
                        BINPREFIX=i686-w64-mingw32-
                        ;;
                    windows-x86_64)
                        BINPREFIX=x86_64-w64-mingw32-
                        ;;
                    *)
                        panic "Unsupported system: $SYSTEM"
                        ;;
                esac
                var_append EXTRA_CONFIGURE_FLAGS \
                    -xplatform win32-g++ \
                    -device-option CROSS_COMPILE=$BINPREFIX \
                    -nomake examples \
                    -nomake tests \
                    -no-warnings-are-errors
                ;;
        esac

        build_qt_package qt-base \
                $EXTRA_CONFIGURE_FLAGS

        for SUBDIR in bin include lib mkspecs plugins; do
            if [ -d "$(builder_install_prefix)/$SUBDIR" ]; then
                copy_directory \
                        "$(builder_install_prefix)/$SUBDIR" \
                        "$INSTALL_DIR/$SYSTEM/$SUBDIR"
            fi
        done

        timestamp_set "$INSTALL_DIR/$SYSTEM" qt

    ) || panic "[$SYSTEM] Could not build Qt!"

done

log "Done building Qt."
