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

# We don't know yet how to build Darwin Mesa binaries. As such, ignore the
# value of ANDROID_EMULATOR_DARWIN_SSH for now. If --darwin-ssh=<hostname>
# is used, the script will fail with an error explaining the reason though.
unset ANDROID_EMULATOR_DARWIN_SSH

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

package_builder_process_options mesa

if [ "$OPT_INSTALL_DIR" ]; then
    INSTALL_DIR=$OPT_INSTALL_DIR
    log "Using install dir: $INSTALL_DIR"
else
    INSTALL_DIR=$PREBUILTS_DIR/$DEFAULT_INSTALL_SUBDIR
    log "Auto-config: --install-dir=$INSTALL_DIR  [default]"
fi

MESA_DEPS_INSTALL_DIR=$PREBUILTS_DIR/mesa-deps

ARCHIVE_DIR=$PREBUILTS_DIR/archive
if [ ! -d "$ARCHIVE_DIR" ]; then
    panic "Missing archive directory: $ARCHIVE_DIR"
fi
if [ ! -d "$ARCHIVE_DIR" ]; then
    panic "Missing archive directory: $ARCHIVE_DIR"
fi
PACKAGE_LIST=$ARCHIVE_DIR/PACKAGES.TXT
if [ ! -f "$PACKAGE_LIST" ]; then
    panic "Missing package list file, run download-sources.sh: $PACKAGE_LIST"
fi

SCONS=$(which scons 2>/dev/null || true)
if [ -z "$SCONS" ]; then
    panic "Please install the 'scons' build tool to use this script!"
fi

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

# Rebuild dependencies if needed.
# $1: System name (e..g darwin-x86_64)
check_mesa_dependencies () {
    local SYSTEM="$1"
    if ! timestamp_check "$MESA_DEPS_INSTALL_DIR/$SYSTEM" mesa-deps; then
        dump "[$SYSTEM] Need to rebuild Mesa dependencies first."
        local EXTRA_FLAGS=
        local VERBOSITY=$(get_verbosity)
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
            --install-dir=$MESA_DEPS_INSTALL_DIR \
            $EXTRA_FLAGS
    fi
}

# Perform a Darwin build through ssh to a remote machine.
# $1: Darwin host name.
# $2: List of darwin target systems to build for.
do_remote_darwin_build () {
    local DARWIN_SYSTEMS="$2"
    local SYSTEM
    for SYSTEM in $DARWIN_SYSTEMS; do
        check_mesa_dependencies "$SYSTEM"
    done
    builder_prepare_remote_darwin_build \
            "/tmp/$USER-rebuild-darwin-ssh-$$/mesa-build"

    local PKG_DIR="$DARWIN_PKG_DIR"
    local REMOTE_DIR=/tmp/$DARWIN_PKG_NAME

    run mkdir -p "$PKG_DIR/prebuilts"
    for SYSTEM in $DARWIN_SYSTEMS; do
        copy_directory "$INSTALL_DIR/$SYSTEM" \
                "$PKG_DIR/prebuilts/mesa/$SYSTEM"
    done
    copy_directory "$ARCHIVE_DIR" "$PKG_DIR"/prebuilts/archive

    if [ "$OPT_OSMESA" ]; then
        var_append DARWIN_BUILD_FLAGS "--osmesa"
    fi

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
    --prebuilts-dir=$REMOTE_DIR/prebuilts \\
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

        timestamp_set "$INSTALL_DIR/$SYSTEM" mesa
    done
}

if [ -z "$OPT_FORCE" ]; then
    builder_check_all_timestamps "$INSTALL_DIR" mesa
fi

if [ "$DARWIN_SYSTEMS" -a "$DARWIN_SSH" ]; then
    # TODO(digit): Implement Mesa darwin builds?
    panic "Sorry, I don't know how to build Darwin Mesa binaries for now!"

    # Building OSMesa remotely.
    dump "Remote Mesa build for: $DARWIN_SYSTEMS"
    do_remote_darwin_build "$DARWIN_SSH" "$DARWIN_SYSTEMS"
fi


for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    check_mesa_dependencies "$SYSTEM"
    (
        export PKG_CONFIG_PATH=$INSTALL_DIR/$SYSTEM/lib/pkgconfig

        BUILD_OSMESA=

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
                OSMESA_TARGET="libgl-osmesa"
                MESA_LIBS="opengl32.dll osmesa.dll"
                BUILD_OSMESA=true
                ;;

            linux-*)
                # 'libgl-xlib' is the target that implements desktop GL
                # as well as the GLX API on top of standard XLib.
                # This generates a libGL.so.
                MESA_TARGET="libgl-xlib"
                OSMESA_TARGET="libgl-osmesa"
                MESA_LIBS="libGL.so libosmesa.so"
                BUILD_OSMESA=true
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
                export LLVM=$MESA_DEPS_INSTALL_DIR/$(builder_host) &&
                run scons -j$NUM_JOBS -C "$PKG_SRC_DIR" \
                    build=release \
                    llvm=yes \
                    machine=$LLVM_TARGETS \
                    $EXTRA_FLAGS \
                    $MESA_TARGET || exit $?

                if [ "$BUILD_OSMESA" ]; then
                    run scons -j$NUM_JOBS -C "$PKG_SRC_DIR" \
                        build=release \
                        llvm=yes \
                        machine=$LLVM_TARGETS \
                        $EXTRA_FLAGS \
                        $OSMESA_TARGET || exit $?
                fi

                # Now copy the result to temporary install directory.
                run mkdir -p "$(builder_install_prefix)/lib" &&
                for LIB in $MESA_LIBS; do
                    case $LIB in
                        *osmesa*)
                            GALLIUM_TARGET="osmesa"
                            ;;
                        *)
                            GALLIUM_TARGET=$MESA_TARGET
                            ;;
                    esac
                    run cp -f \
                        "$PKG_SRC_DIR"/build/$(builder_host)/gallium/targets/$GALLIUM_TARGET/$LIB \
                        "$(builder_install_prefix)/lib/$LIB"
                done

            ) || panic "Could not build $SYSTEM Mesa from sources"

            timestamp_set "$PKG_BUILD_DIR" "$PKG_NAME"
        fi

        # Copy to install location.
        run mkdir -p "$INSTALL_DIR"/$SYSTEM/lib
        for LIB in $MESA_LIBS; do
            run cp -f "$(builder_install_prefix)/lib/$LIB" \
                    "$INSTALL_DIR/$SYSTEM/lib/$LIB"
        done

    ) || panic "[$SYSTEM] Could not build mesa!"

done

dump "Done."
