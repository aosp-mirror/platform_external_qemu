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

# Clone a git repository, and checkout a specific branch & commit.
# $1: Source git URL.
# $2: Destination directory.
# $3: Branch / Commit
git_clone () {
    local GIT_URL DST_DIR BRANCH CHECK_REV
    GIT_URL=$1
    DST_DIR=$2
    BRANCH=$3

    if [ -d "$DST_DIR" ]; then
        panic "Git destination directory already exists: $DST_DIR"
    fi
    run git clone "$GIT_URL" "$DST_DIR" ||
            panic "Could not clone git repository: $GIT_URL"
    run git -C "$DST_DIR" checkout $BRANCH ||
            panic "Could not checkout $GIT_URL - $BRANCH"
}

# Build the ANGLE packages.
#
# $1: Source directory.
# $2: The directory the output libraries are placed in.
build_angle_package () {
    local PKG_FULLNAME="$(basename "$1")"
    dump "$(builder_text) Building $PKG_FULLNAME"

    local PKG_BUILD_DIR="$1"
    local PKG_LIB_DIR="$2"

    # Bolierplate setup
    run mkdir -p "$PKG_BUILD_DIR" &&
    run cd "$PKG_BUILD_DIR" &&
    export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
    export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
    export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
    export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&

    # Ensure the gclient command is available.
    git_clone https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools c55ba20c629ef702cd4bb06da9235c4bb7217f96 &&
    export PATH=`pwd`/depot_tools:"$PATH" &&

    # Ensure ninja, included in the depot tools, is used to generator the build
    # files, since it is a cross-platform solution.
    export GYP_GENERATORS=ninja &&

    # Create gclient file and sync the build files
    run python scripts/bootstrap.py &&

    # Sync the appropriate build files
    # The first sync fails, however the second will work
    run gclient sync || run gclient sync &&

    # ninja is provided for each platform in the previously cloned depot_tools
    run ninja -C out/Debug

    # The static libs produced by the build script are just thin archives so
    # they don't contain any objects themselves -> repackage them to actually
    # contain the object files
    for lib in $PKG_LIB_DIR/lib*.a; do
        ar -t $lib | run xargs ar rvs $lib.new && mv $lib.new $lib;
        run ranlib $lib
    done
}

# Build the ANGLE packages using mingw.
# Based on the https://github.com/Martchus/PKGBUILDs/blob/master/angleproject/mingw-w64/PKGBUILD
#
# $1: Source directory.
# $2: The directory the output libraries are placed in.
mingw_build_angle_package() {
    local PKG_FULLNAME="$(basename "$1")"
    dump "$(builder_text) Building $PKG_FULLNAME"

    local PKG_BUILD_DIR="$1"
    local PKG_LIB_DIR="$2"

    run mkdir -p "$PKG_BUILD_DIR" &&
    run cd "$PKG_BUILD_DIR"

    # Setup the right version of mingw
    local MINGW_PREFIX GYP_TARGET
    case $SYSTEM in
        windows-x86)
            MINGW_PREFIX=i686
            GYP_TARGET=win32
            $AOSP_DIR/external/qemu/android/scripts/gen-android-sdk-toolchain.sh \
                --host=windows-x86 \
                ./mingw-toolchain
            ;;
        windows-x86_64)
            MINGW_PREFIX=x86_64
            GYP_TARGET=win64
            $AOSP_DIR/external/qemu/android/scripts/gen-android-sdk-toolchain.sh \
                --host=windows-x86_64 \
                ./mingw-toolchain
            ;;
    esac
    export PATH=`pwd`/mingw-toolchain:"$PATH"

    # Boilerplate setup
    export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
    export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
    export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
    export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&

    # Ensure the gyp command is available.
    git_clone https://chromium.googlesource.com/external/gyp gyp bac4680ec9a5c55ab692490b6732999648ecf1e9 &&
    export PATH=`pwd`/gyp:"$PATH" &&

    # Provide 32-bit versions of *.def files
    cp mingw-w64/libEGL_mingw32.def src/libEGL/ &&
    cp mingw-w64/libGLESv2_mingw32.def src/libGLESv2/ &&

    # Provide a file to export symbols declared in ShaderLang.h as part of libGLESv2.dll
    # (required to build Qt WebKit which uses shader interface)
    cp mingw-w64/entry_points_shader.cpp src/libGLESv2/ &&

    # Remove .git directory to prevent:
    # No rule to make target '../build-i686-w64-mingw32/.git/index', needed by 'out/Debug/obj/gen/angle/id/commit.h'.
    rm -rf .git &&

    # Make sure an import library is created, the correct .def file is used
    # during the build and entry_points_shader.cpp is compiled
    run patch -p1 -i "mingw-w64/angleproject-include-import-library-and-use-def-file.patch" &&

    # Provide own implementation of mbstowcs_s for Windows XP support
    run patch -p1 -i "mingw-w64/provide_mbstowcs_s_for_xp.patch" &&

    # Executing .bat scripts on Linux is a no-go so make this a no-op
    echo "" > src/copy_compiler_dll.bat &&
    chmod +x src/copy_compiler_dll.bat &&

    # Set build flags, make sure all header files are found
    local ARCHFLAG=""
    case $SYSTEM in
        windows-x86)
            ARCHFLAG=" -m32"
            ;;
        windows-x86_64)
            ARCHFLAG=" -m64"
            ;;
    esac

    export CXXFLAGS="-O2 -g $ARCHFLAG -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 -std=c++11 -msse2 -DUNICODE -D_UNICODE -g -Isrc -Iinclude" &&
    export CXX="$MINGW_PREFIX-w64-mingw32-g++" &&
    export AR="$MINGW_PREFIX-w64-mingw32-ar" &&

    # TODO: re-enable building ALL of angle, not just the static libraries
    # gyp -D use_ozone=0 -D OS=win -D TARGET=$GYP_TARGET --format make -D \
    #     MSVS_VERSION="" --depth . -I build/common.gypi src/angle.gyp &&

    # # Forcing non-concurrent build to prevent failure
    # run make -j1 V=1 &&

    # static libs must be built separately
    run gyp -D use_ozone=0 -D OS=win -D TARGET=$GYP_TARGET --format make -D \
        MSVS_VERSION="" --depth . -I build/common.gypi src/angle.gyp -D \
        angle_gl_library_type=static_library &&
    run make -j$NUM_JOBS V=1

    # The static libs produced by the build script are just thin archives so
    # they don't contain any objects themselves -> repackage them to actually
    # contain the object files
    for lib in $PKG_LIB_DIR/lib*.a; do
        ar -t $lib | run xargs ar rvs $lib.new && mv $lib.new $lib;
        run ranlib $lib
    done
}

# Unpack package source into $(builder_src_dir) if needed.
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
    local PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP PKG_LIB_DIR
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/angle
    copy_angle_source
    PKG_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/angle-timestamp
    if [ ! -f "$PKG_TIMESTAMP" -o -n "$OPT_FORCE" ]; then
        case $SYSTEM in
            win*)
                PKG_LIB_DIR=$PKG_BUILD_DIR/out/Debug/obj.target/src/
                ;;
            linux*)
                PKG_LIB_DIR=$PKG_BUILD_DIR/out/Debug/obj/src/
                ;;
            darwin*)
                # Required for proper build on Darwin!
                PKG_LIB_DIR=$PKG_BUILD_DIR/out/Debug/
                builder_disable_verbose_install
                ;;
        esac

        case $SYSTEM in
            win*)
                mingw_build_angle_package \
                    "$PKG_BUILD_DIR" \
                    "$PKG_LIB_DIR" \
                    "$@"
                ;;
            *)
                build_angle_package \
                    "$PKG_BUILD_DIR" \
                    "$PKG_LIB_DIR" \
                    "$@"
                ;;
        esac

        # We don't have a make install command at all since the Makefile is
        # auto-generated, so we just copy the files we want directly.
        copy_directory_files \
                    "$PKG_LIB_DIR" \
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

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        build_angle_libraries

        # Copy binaries necessary for the build itself as well as static libraries.
        copy_directory \
                "$(builder_install_prefix)/" \
                "$INSTALL_DIR/$SYSTEM/"

        dump "[$SYSTEM] Done building ANGLE libraries"

    ) || panic "[$SYSTEM] Could not build ANGLE libraries!"

done

log "Done building ANGLE libraries."
