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
"A script used to rebuild the Mesa software GL graphics library from sources."

# NOTE: Cross-building for windows doesn't work yet, so ensure that
#       on Linux, the default host systems is only 'linux-x86_64'
case $(get_build_os) in
    XXlinux)
        DEFAULT_HOST_SYSTEMS="linux-x86_64"
        ;;
esac

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option mesa

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

ARCHIVE_DIR=$PREBUILTS_DIR/archive
if [ ! -d "$ARCHIVE_DIR" ]; then
    panic "Missing archive directory: $ARCHIVE_DIR"
fi
PACKAGE_LIST=$ARCHIVE_DIR/PACKAGES.TXT
if [ ! -f "$PACKAGE_LIST" ]; then
    panic "Missing package list file, run download-sources.sh: $PACKAGE_LIST"
fi


package_builder_process_options mesa

if [ "$OPT_INSTALL_DIR" ]; then
    INSTALL_DIR=$OPT_INSTALL_DIR
    log "Using install dir: $INSTALL_DIR"
else
    INSTALL_DIR=$PREBUILTS_DIR/$DEFAULT_INSTALL_SUBDIR
    log "Auto-config: --install-dir=$INSTALL_DIR  [default]"
fi

package_list_parse_file "$PACKAGE_LIST"

# Check that we have the right prebuilts
EXTRA_FLAGS=
if [ "$DARWIN_SSH" ]; then
    var_append EXTRA_FLAGS "--darwin-ssh=$DARWIN_SSH"
fi
$(program_directory)/build-mesa-deps.sh \
    --verbosity=$(get_verbosity) \
    --prebuilts-dir=$PREBUILTS_DIR \
    --aosp-dir=$AOSP_DIR \
    --host=$(spaces_to_commas "$HOST_SYSTEMS") \
    $EXTRA_FLAGS \
        || panic "could not check or rebuild mesa dependencies."

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

# $1: Package basename (e.g. 'libpthread-stubs-0.3')
# $2+: Extra configuration options.
build_package () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_NAME=$(package_list_get_src_dir $1)
    unpack_package_source "$1"
    shift
    PKG_SRC_DIR="$BUILD_SRC_DIR/$PKG_NAME"
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/$PKG_NAME
    if ! timestamp_check "$PKG_BUILD_DIR" "$PKG_NAME"; then
        builder_build_autotools_package \
            "$PKG_SRC_DIR" \
            "$PKG_BUILD_DIR" \
            "$@"

        timestamp_set "$PKG_BUILD_DIR" "$PKG_NAME"
    fi
}

build_windows_mesa () {
    # TODO(digit): MAKE THIS WORK CORRECTLY!!

    # panic "Building offscreen Mesa for Windows doesn't work yet!"

    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_NAME=$(package_list_get_src_dir MesaLib)
    unpack_package_source "MesaLib"
    shift
    PKG_SRC_DIR="$BUILD_SRC_DIR/$PKG_NAME"
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/$PKG_NAME
    PKG_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/$PKG_NAME-timestamp
    if [ ! -f "$PKG_TIMESTAMP" -o -n "$OPT_FORCE" ]; then
        (
            export LDFLAGS="-static -s" &&
            export CXXFLAGS="-std=gnu++11" &&
            export LLVM=$INSTALL_DIR/$(builder_host) &&
            mkdir -p "$PKG_BUILD_DIR" &&
            cd "$PKG_BUILD_DIR" &&
            run scons -j$NUM_JOBS -C "$PKG_SRC_DIR" \
                build=release \
                platform=windows \
                toolchain=crossmingw \
                llvm=yes \
                machine=$LLVM_TARGETS \
                libgl-gdi
        ) || panic "Could not build Windows Mesa from sources"

        touch "$PKG_TIMESTAMP"
    fi
}

build_linux_mesa () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    local BINPREFIX=$(builder_gnu_config_host_prefix)
    local EXTRA_FLAGS
    PKG_NAME=$(package_list_get_src_dir MesaLib)
    unpack_package_source "MesaLib"
    shift
    PKG_SRC_DIR="$BUILD_SRC_DIR/$PKG_NAME"
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/$PKG_NAME
    PKG_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/$PKG_NAME-timestamp
    if [ "$(get_verbosity)" -gt 2 ]; then
        var_append EXTRA_FLAGS "verbose=true"
    fi
    if ! timestamp_check "$PKG_BUILD_DIR" "$PKG_NAME"; then
        (
            export LLVM=$INSTALL_DIR/$(builder_host) &&
            mkdir -p "$PKG_BUILD_DIR" &&
            cd "$PKG_BUILD_DIR" &&
            run scons -j$NUM_JOBS -C "$PKG_SRC_DIR" \
                build=release \
                llvm=yes \
                machine=$LLVM_TARGETS \
                libgl-xlib \
                $EXTRA_FLAGS &&

            # The SCons build scripts place everything under $SRC/build
            # so copy the final binaries to our installation directory.
            run mkdir -p "$(builder_install_prefix)/lib" &&
            run cp -f \
                "$PKG_SRC_DIR"/build/$(builder_host)/gallium/targets/libgl-xlib/lib* \
                "$(builder_install_prefix)/lib/"

        ) || panic "Could not build Linux Mesa from sources"

        timestamp_set "$PKG_BUILD_DIR" "$PKG_NAME"
    fi
}

# TODO(digit): Implement Mesa darwin builds?
if [ "$DARWIN_SYSTEMS" ]; then
    panic "Sorry, I don't know how to build Darwin Mesa binaries for now!"
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    # Rebuild dependencies if needed.
    if ! timestamp_check "$INSTALL_DIR/$SYSTEM" mesa-deps; then
        dump "[$SYSTEM] Need to rebuild Mesa dependencies first."
        EXTRA_FLAGS=
        VERBOSITY=$(get_verbosity)
        while [ "$VERBOSITY" -gt 1 ]; do
            var_append EXTRA_FLAGS "--verbose"
            VERBOSITY=$(( $VERBOSITY - 1 ))
        done
        if [ "$DARWIN_SSH" ]; then
            var_append EXTRA_FLAGS "--darwin-ssh=$DARWIN_SSH"
        fi
        $(program_directory)/build-mesa-deps.sh \
            --aosp-dir=$AOSP_DIR \
            --prebuilts-dir=$PREBUILTS_DIR \
            --build-dir=$BUILD_DIR \
            --host=$SYSTEM \
            --install-dir=$INSTALL_DIR \
            $EXTRA_FLAGS
    fi

    (
        export PKG_CONFIG_PATH=$INSTALL_DIR/$SYSTEM/lib/pkgconfig

        builder_prepare_for_host_no_binprefix \
                "$SYSTEM" \
                "$AOSP_DIR" \
                "$INSTALL_DIR/$SYSTEM"

        builder_reset_configure_flags \
                --disable-static \
                --with-pic

        case $SYSTEM in
            *-x86)
                LLVM_TARGETS=x86
                ;;
            *-x86_64)
                LLVM_TARGETS=x86_64
                ;;
            *)
                panic "Unknown system CPU architecture: $SYSTEM"
        esac

        PKG_NAME=$(package_list_get_src_dir MesaLib)

        # Unpack sources if necessary.
        PKG_SRC_DIR="$BUILD_SRC_DIR/$PKG_NAME"
        if ! timestamp_check "$BUILD_SRC_DIR" "$PKG_NAME"; then
            unpack_package_source "MesaLib"
            timestamp_set "$BUILD_SRC_DIR" "$PKG_NAME"
        fi

        # The SCons build scripts put everything under $PKG_SRC_DIR/build
        # So clear that up just in case.
        run rm -rf "$PKG_SRC_DIR"/build/*

        EXTRA_FLAGS=
        case $SYSTEM in
            windows-*)
                export LDFLAGS="-static -s"
                export CXXFLAGS="-std=gnu++11"
                var_append EXTRA_FLAGS "platform=windows toolchain=crossmingw"
                # 'libgl-gdi' is the target that implements desktop GL
                # on top of the Win32 GDI interface. This generates a
                # library called opengl32.dll to match the platform's
                # official OpenGL shared library name.
                MESA_TARGET="libgl-gdi"
                MESA_LIBS="opengl32.dll"
                ;;

            linux-*)
                # 'libgl-xlib' is the target that implements desktop GL
                # as well as the GLX API on top of standard XLib.
                # This generates a libGL.so.
                MESA_TARGET="libgl-xlib"
                MESA_LIBS="libGL.so"
                ;;

            *)
                panic "Don't know how to generate Mesa for this system: $SYSTEM"
                ;;
        esac

        if [ "$(get_verbosity)" -gt 2 ]; then
            var_append EXTRA_FLAGS "verbose=true"
        fi

        PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/$PKG_NAME
        if ! timestamp_check "$PKG_BUILD_DIR" "$PKG_NAME"; then
            (
                dump "$(builder_text) Building $PKG_NAME"
                case $SYSTEM in
                    windows-*)
                        export LDFLAGS="-static -s"
                        export CXXFLAGS="-std=gnu++11"
                        ;;
                esac
                export LLVM=$INSTALL_DIR/$(builder_host) &&
                run scons -j$NUM_JOBS -C "$PKG_SRC_DIR" \
                    build=release \
                    llvm=yes \
                    machine=$LLVM_TARGETS \
                    $EXTRA_FLAGS \
                    $MESA_TARGET

                # Now copy the result to temporary install directory.
                run mkdir -p "$(builder_install_prefix)/lib" &&
                for LIB in $MESA_LIBS; do
                    run cp -f \
                        "$PKG_SRC_DIR"/build/$(builder_host)/gallium/targets/$MESA_TARGET/$LIB \
                        "$(builder_install_prefix)/lib/$LIB"
                done

            ) || panic "Could not build $SYSTEM Mesa from sources"

            timestamp_set "$PKG_BUILD_DIR" "$PKG_NAME"
        fi

        # Copy to install location.
        for LIB in $MESA_LIBS; do
            run cp -f "$(builder_install_prefix)/lib/$LIB" \
                    "$INSTALL_DIR/$SYSTEM/lib/$LIB"
        done

    ) || panic "[$SYSTEM] Could not build mesa!"

done

dump "Done."
