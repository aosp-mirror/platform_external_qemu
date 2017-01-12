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
package_builder_parse_package_list

MESA_DEPS_INSTALL_DIR=$PREBUILTS_DIR/mesa-deps

SCONS=$(which scons 2>/dev/null || true)
if [ -z "$SCONS" ]; then
    panic "Please install the 'scons' build tool to use this script!"
fi

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

if [ -z "$OPT_FORCE" ]; then
    builder_check_all_timestamps "$INSTALL_DIR" mesa
fi

if [ "$DARWIN_SYSTEMS" -a "$DARWIN_SSH" ]; then
    # TODO(digit): Implement Mesa darwin builds?
    panic "Sorry, I don't know how to build Darwin Mesa binaries for now!"

    # Building OSMesa remotely.
    dump "Remote Mesa build for: $DARWIN_SYSTEMS"
    for SYSTEM in $DARWIN_SYSTEMS; do
        check_mesa_dependencies "$SYSTEM"
    done

    if [ "$OPT_OSMESA" ]; then
        var_append DARWIN_BUILD_FLAGS "--osmesa"
    fi

    builder_prepare_remote_darwin_build

    run mkdir -p "$DARWIN_PKG_DIR/prebuilts"
    for SYSTEM in $DARWIN_SYSTEMS; do
        copy_directory "$INSTALL_DIR/$SYSTEM" \
                "$DARWIN_PKG_DIR/prebuilts/mesa/$SYSTEM"
    done

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
        timestamp_set "$INSTALL_DIR/$SYSTEM" mesa
    done
fi


for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    check_mesa_dependencies "$SYSTEM"
    (
        export PKG_CONFIG_PATH=$INSTALL_DIR/$SYSTEM/lib/pkgconfig

        BUILD_OSMESA=

        builder_enable_cxx11

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
        PKG_SRC_DIR="$(builder_src_dir)/$PKG_NAME"
        if ! timestamp_check "$(builder_src_dir)" "$PKG_NAME"; then
            builder_unpack_package_source "MesaLib"
            timestamp_set "$(builder_src_dir)" "$PKG_NAME"
        fi

        # The SCons build scripts put everything under $PKG_SRC_DIR/build
        # So clear that up just in case.
        run rm -rf "$PKG_SRC_DIR"/build/*

        EXTRA_FLAGS=
        case $SYSTEM in
            windows-*)
                export LDFLAGS="-static -s -Xlinker --build-id"
                export CXXFLAGS="-std=gnu++11"
                var_append EXTRA_FLAGS "platform=windows toolchain=crossmingw"
                # 'libgl-gdi' is the target that implements desktop GL
                # on top of the Win32 GDI interface. This generates a
                # library called opengl32.dll to match the platform's
                # official OpenGL shared library name.
                MESA_TARGET="libgl-gdi"
                OSMESA_TARGET="libgl-osmesa"
                MESA_LIBS="opengl32.dll"
                BUILD_OSMESA=true
                ;;

            linux-*)
                # 'libgl-xlib' is the target that implements desktop GL
                # as well as the GLX API on top of standard XLib.
                # This generates a libGL.so.
                MESA_TARGET="libgl-xlib"
                OSMESA_TARGET="libgl-osmesa"
                MESA_LIBS="libGL.so"
                BUILD_OSMESA=true
                ;;

            *)
                panic "Don't know how to generate Mesa for this system: $SYSTEM"
                ;;
        esac

        if [ "$(get_verbosity)" -gt 2 ]; then
            var_append EXTRA_FLAGS "verbose=true"
        fi

        PKG_BUILD_DIR=$(builder_build_dir)/$PKG_NAME
        if ! timestamp_check "$PKG_BUILD_DIR" "$PKG_NAME"; then
            (
                dump "$(builder_text) Building $PKG_NAME"
                case $SYSTEM in
                    windows-*)
                        export LDFLAGS="-static -Xlinker --build-id"
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

        case $SYSTEM in
            linux-*)
                # Special handling for Linux, this is needed because QT
                # will actually try to load libGL.so.1 before GPU emulation
                # is initialized. This is actually a link to the system's
                # libGL.so, and will thus prevent the Mesa version from
                # loading. By creating the symlink, Mesa will also be used
                # by QT.
                ln -sf libGL.so "$INSTALL_DIR/$SYSTEM/lib/libGL.so.1"
                ;;
        esac

    ) || panic "[$SYSTEM] Could not build mesa!"

done

dump "Done."
