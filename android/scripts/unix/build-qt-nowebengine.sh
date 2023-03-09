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
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt Qt host libraries.

This script is used to download the Qt source tarball from source, then
extract it and build the Qt host tools and libraries.
"

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option qt
package_builder_register_options

OPT_DOWNLOAD=
option_register_var "--download" OPT_DOWNLOAD "Download source tarball."

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options qt
package_builder_parse_package_list

###
###  Download the source tarball if needed.
###
QT_SRC_NAME=qt-everywhere-src-5.12.1
QT_SRC_PACKAGE=$QT_SRC_NAME.tar.xz
QT_SRC_URL=http://download.qt.io/archive/qt/5.12/5.12.1/single/$QT_SRC_PACKAGE
QT_SRC_PACKAGE_SHA1=SHA1=9377d37cdde14ba2009d4f2077986a8910d03ee7

QT_SRC_PATCH_FOLDER=${QT_SRC_NAME}-patches
QT_ARCHIVE_DIR=$(builder_archive_dir)

if [ -z "$OPT_DOWNLOAD" -a ! -f "$QT_ARCHIVE_DIR/$QT_SRC_PACKAGE" ]; then
    if [ -z "$OPT_DOWNLOAD" ]; then
        echo "The following tarball is missing: $QT_ARCHIVE_DIR/$QT_SRC_PACKAGE"
        echo "Please use the --download option to download it. Note that this is"
        echo "a huge file over 300 MB, the download will take time."
        exit 1
    fi
fi

# Download a file.
# $1: Source URL
# $2: Destination directory.
# $3: [Optional] expected SHA-1 sum of downloaded file.
download_package () {
    # Assume the packages are already downloaded under $QT_ARCHIVE_DIR
    local DST_DIR PKG_URL PKG_NAME SHA1SUM REAL_SHA1SUM

    PKG_URL=$1
    PKG_NAME=$(basename "$PKG_URL")
    DST_DIR=$2
    SHA1SUM=$3

    log "Downloading $PKG_NAME..."
    (cd "$DST_DIR" && run curl -L -o "$PKG_NAME" "$PKG_URL") ||
            panic "Can't download '$PKG_URL'"

    if [ "$SHA1SUM" ]; then
        log "Checking $PKG_NAME content"
        REAL_SHA1SUM=$(compute_file_sha1 "$DST_DIR"/$PKG_NAME)
        if [ "$REAL_SHA1SUM" != "$SHA1SUM" ]; then
            panic "Error: Invalid SHA-1 sum for $PKG_NAME: $REAL_SHA1SUM (expected $SHA1SUM)"
        fi
    fi
}

if [ "$OPT_DOWNLOAD" ]; then
    download_package "$QT_SRC_URL" "$QT_ARCHIVE_DIR" "$QT_SRC_PACKAGE_SHA1"
fi

# Atomically update target directory $1 with the content of $2.
# This also removes $2 on success.
# $1: target directory.
# $2: source directory.
directory_atomic_update () {
    local DST_DIR="$1"
    local SRC_DIR="$2"
    if [ -d "$DST_DIR" ]; then
        run rm -f "$DST_DIR".old &&
        run mv "$DST_DIR" "$DST_DIR".old
    fi
    run mv "$SRC_DIR" "$DST_DIR" &&
    run rm -rf "$DST_DIR".old
}

# $1: Package name (e.g. 'qt-base')
# $2: List of extra modules to build (e.g. 'qtsvg')
# $3+: Extra configuration options.
build_qt_package () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    local PKG_MODULES
    PKG_NAME=$(package_list_get_src_dir $1)
    PKG_MODULES=$2
    shift; shift
    PKG_SRC_DIR=$(builder_src_dir)/$PKG_NAME
    PKG_BUILD_DIR=$(builder_build_dir)/$PKG_NAME
    (
        run mkdir -p "$PKG_BUILD_DIR" &&
        run cd "$PKG_BUILD_DIR" &&
        export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
        export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
        export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
        export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&
        run "$PKG_SRC_DIR"/configure \
            -prefix $_SHU_BUILDER_PREFIX \
            "$@" &&
        run make -j$NUM_JOBS V=1 &&
        run make install -j$NUM_JOBS V=1
    ) ||
    panic "Could not build and install $1"
}

# Our custom qtwebengine dependencies folder also contains some dependencies for
# non-webengine build, so let's include it.
configure_qtwebengine_pkg_config () {
    # Two things are happening here:
    #  1) Copy the .pc files from qtwebengine-deps to
    #  toolchain-wrapper/qtwebengine-pkgconfig,
    #  2) modify the .pc file prefixes to point to the lib folder in
    #  qtwebengine-deps
    local QTWEBENGINE_DEPS_DIR="$PREBUILTS_DIR/common/qtwebengine-deps/linux-x86_64"
    local TOOLCHAIN_PKGCONFIG_DIR="$_SHU_BUILDER_BUILD_DIR/toolchain-wrapper/qtwebengine-pkgconfig"
    run mkdir $TOOLCHAIN_PKGCONFIG_DIR
    run cp $QTWEBENGINE_DEPS_DIR/lib/pkgconfig/*.pc "$TOOLCHAIN_PKGCONFIG_DIR"
    run sed -i "s|\${qtwebengine_deps_dir}|$QTWEBENGINE_DEPS_DIR|g" $TOOLCHAIN_PKGCONFIG_DIR/*.pc
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote Qt build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    run mkdir -p "$INSTALL_DIR" ||
            panic "Could not create final directory: $INSTALL_DIR"

    if [ -d "$INSTALL_DIR"/common/include ]; then
        run rm -rf "$INSTALL_DIR"/common/include.old
        run mv "$INSTALL_DIR"/common/include "$INSTALL_DIR"/common/include.old
    fi

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR &&
        run mkdir -p $INSTALL_DIR/common &&
        builder_remote_darwin_rsync -haz --delete \
                $DARWIN_SSH:$DARWIN_REMOTE_DIR/install-prefix/common/include \
                $INSTALL_DIR/common/
    done

    run rm -rf "$INSTALL_DIR/common/include.old"
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        case $SYSTEM in
            windows*)
                builder_prepare_for_host "$SYSTEM" "$AOSP_DIR"
                ;;
            *)
                case $SYSTEM in
                    linux-x86_64)
                        _SHU_BUILDER_BUILD_DIR=$TEMP_DIR/build-$SYSTEM
                        # make sure our .pc files are found first because
                        # glibc's xcb library is too old (1.8). Qt 5.12.1
                        # requires > 1.9.
                        EXTRA_PKG_CONFIG_PATH=$_SHU_BUILDER_BUILD_DIR/toolchain-wrapper/qtwebengine-pkgconfig
                        ;;
                    *)
                        ;;
                esac
                builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"
                ;;
        esac

        case $SYSTEM in
            linux-x86_64)
                # Add custom pkg-config files pointing to our qtwebengine-deps
                # folder.
                configure_qtwebengine_pkg_config
                ;;
            *)
                ;;
        esac

        # Unpacking the sources if needed.
        PKG_NAME=$(package_list_get_unpack_src_dir "qt")
        ARCHIVE_DIR=$(builder_archive_dir)
        SRC_DIR=$(builder_src_dir)
        TIMESTAMP=$SRC_DIR/timestamp-$PKG_NAME
        if [ ! -f "$TIMESTAMP" -o "$OPT_FORCE" ]; then
            package_list_unpack_and_patch "qt" "$ARCHIVE_DIR" "$SRC_DIR"
            touch $TIMESTAMP
        fi

        # Configuring the build. This takes a lot of time due to QMake.
        dump "$(builder_text) Configuring Qt build"

        LD_LIBRARY_PATH=
        EXTRA_CONFIGURE_FLAGS=
        var_append EXTRA_CONFIGURE_FLAGS \
                -opensource \
                -confirm-license \
                -force-debug-info \
                -release \
                -no-rpath \
                -shared \
                -nomake examples \
                -nomake tests \
                -no-strip \

        # This will add <qtlibinfix> to all the shared library names, i.e.,
        # libQt5*.so --> libQt5*<qtlibinfix>.so
        # We don't want to use the default names on Windows in case the user has
        # some other version of Qt loaded in memory from a different application
        # (DLL hell).
        var_append EXTRA_CONFIGURE_FLAGS \
                -qtlibinfix AndroidEmu

        if [ "$(get_verbosity)" -gt 2 ]; then
            var_append EXTRA_CONFIGURE_FLAGS "-v"
        fi

        case $SYSTEM in
            linux-x86_64)
                var_append EXTRA_CONFIGURE_FLAGS \
                        -qt-xcb \
                        -no-use-gold-linker \
                        -platform linux-clang \

                var_append LD_LIBRARY_PATH \
                  $(dirname $(aosp_clang_libcplusplus))
                ;;
            windows*)
                case $SYSTEM in
                    windows-x86_64)
                        var_append EXTRA_CONFIGURE_FLAGS \
                                -no-opengl
                        BINPREFIX=x86_64-w64-mingw32-
                        ;;
                    *)
                        panic "Unsupported system: $SYSTEM"
                        ;;
                esac
                var_append EXTRA_CONFIGURE_FLAGS \
                    -xplatform win32-g++ \
                    -device-option CROSS_COMPILE=$BINPREFIX \
                    -no-warnings-are-errors
                var_append LDFLAGS "-Xlinker --build-id"

                # Somehow Windows build doesn't generate debug information
                # unless asked explicitly
                var_append CFLAGS -O2 -g
                var_append CXXFLAGS -O2 -g -fno-rtti -fno-exceptions
                ;;
            darwin*)
                # '-sdk macosx' without the version forces the use of the latest
                # one installed on the build machine. To make sure one knows
                # the verion used, let's dump all sdks
                echo "The list of the installed OS X SDKs:"
                xcodebuild -showsdks | grep macosx
                echo "(using the latest version for the Qt build)"

                var_append EXTRA_CONFIGURE_FLAGS \
                    -no-framework \
                    -sdk macosx
                var_append CFLAGS -mmacosx-version-min=10.11
                var_append LDFLAGS -mmacosx-version-min=10.11
                var_append LDFLAGS -lc++
                ;;
        esac

        QT_BUILD_DIR=$(builder_build_dir)

        var_append LDFLAGS "-L$_SHU_BUILDER_PREFIX/lib"
        var_append CPPFLAGS "-I$_SHU_BUILDER_PREFIX/include"

        (
            run mkdir -p "$QT_BUILD_DIR" &&
            run cd "$QT_BUILD_DIR" &&
            export CFLAGS &&
            export CXXFLAGS &&
            export LDFLAGS &&
            export CPPFLAGS &&
            export LD_LIBRARY_PATH &&
            export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
            export PKG_CONFIG_PATH="$EXTRA_PKG_CONFIG_PATH:$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&
            run "$(builder_src_dir)"/$QT_SRC_NAME/configure \
                -prefix $_SHU_BUILDER_PREFIX \
                $EXTRA_CONFIGURE_FLAGS
        ) || panic "Could not configure Qt build!"

        # Build everything now.
        QT_MODULES="qtbase qtsvg qtimageformats"
        QT_TARGET_BUILD_MODULES=
        QT_TARGET_INSTALL_MODULES=
        for QT_MODULE in $QT_MODULES; do
            var_append QT_TARGET_BUILD_MODULES "module-$QT_MODULE"
            var_append QT_TARGET_INSTALL_MODULES "module-$QT_MODULE-install_subtargets"
        done

        QT_MAKE_FLAGS="-j$NUM_JOBS"
        if [ "$(get_verbosity)" -gt 2 ]; then
            var_append QT_MAKE_FLAGS "V=1"
        fi

        dump "$(builder_text) Building Qt binaries"
        (
            run cd "$QT_BUILD_DIR" &&
            export LD_LIBRARY_PATH &&
            run make $QT_MAKE_FLAGS $QT_TARGET_BUILD_MODULES
        ) || panic "Could not build Qt binaries!"

        dump "$(builder_text) Installing Qt binaries"
        (
            run cd "$QT_BUILD_DIR" &&
            export LD_LIBRARY_PATH &&
            run make $QT_MAKE_FLAGS $QT_TARGET_INSTALL_MODULES
        ) || panic "Could not install Qt binaries!"

        # Find all Qt static libraries.
        case $SYSTEM in
            darwin*)
                QT_DLL_FILTER="*.dylib"
                ;;
            windows*)
                QT_DLL_FILTER="*.dll"
                ;;
            *)
                QT_DLL_FILTER="*.so*"
                ;;
        esac
        QT_SHARED_LIBS=$(cd "$(builder_install_prefix)" && \
                find . -name "$QT_DLL_FILTER" 2>/dev/null)
        if [ -z "$QT_SHARED_LIBS" ]; then
            panic "Could not find any Qt shared library!!"
        fi

        # Let's fix up the rpaths in darwin. This allows binaries that link
        # against qt to launch directly if they contain a proper rpath. This
        # removes the need to set DYLD_LIBRARY_PATH properly.
        (
            # We don't want any symlinks here
            QT_HARD_SHARED_LIBS=$(cd "$(builder_install_prefix)" && \
                    find . -type f -name "$QT_DLL_FILTER" 2>/dev/null)
            run cd "$(builder_install_prefix)"
            case $SYSTEM in
                darwin*)
                    for lib in $QT_HARD_SHARED_LIBS; do
                        echo "Fixing $lib"
                        idfix=$(basename $lib)
                        echo "\t$(otool -DX $lib) ==> @rpath/$idfix"
                        run install_name_tool -id "@rpath/$idfix" $lib
                        # change whatever Qt libs this lib uses to rpaths
                        tofix=$(otool -LX $lib | grep "libQt.*.dylib" | cut -f1 -d ' ')
                        for fix in $tofix; do
                            fix_basename=$(basename $fix)
                            # Always point to the hard-link (libQt5*.<3-digit-ver>.dylib)
                            qt_version="5.12.1"
                            fix_basename="${fix_basename%%.*}.$qt_version.dylib"
                            echo "\t$fix ==> @rpath/$fix_basename"
                            run install_name_tool -change "$fix" "@rpath/$fix_basename" $lib
                        done
                    done

                    # Also need to change the shared Qt libs used to rpaths in the executables we use
                    var_append fix_bins \
                            "$(builder_install_prefix)/bin/moc" \
                            "$(builder_install_prefix)/bin/rcc" \
                            "$(builder_install_prefix)/bin/uic"

                    for bin in $fix_bins; do
                        echo "Fixing $bin"
                        # change whatever Qt libs this binary uses to rpaths
                        tofix=$(otool -LX $bin | grep "libQt.*.dylib" | cut -f1 -d ' ')
                        for fix in $tofix; do
                            fix_basename=$(basename $fix)
                            echo "\t$fix ==> @rpath/$fix_basename"
                            run install_name_tool -change "$fix" "@rpath/$fix_basename" $bin
                        done
                    done
                    # setup the rpaths so we can use them without setting environment variables.
                    run install_name_tool -add_rpath "@loader_path/../lib" "$(builder_install_prefix)/bin/moc"
                    run install_name_tool -add_rpath "@loader_path/../lib" "$(builder_install_prefix)/bin/rcc"
                    run install_name_tool -add_rpath "@loader_path/../lib" "$(builder_install_prefix)/bin/uic"
                    ;;
                linux-x86_64)
                    # Use "ldd <binary>" to check if binaries like bin/moc can find
                    # libc++.so.1
                    # The following requires "patchelf" on your system to add rpaths
                    # to the binaries (sudo apt-get install patchelf).
                    run patchelf --set-rpath '$ORIGIN/../lib' "$(builder_install_prefix)/bin/moc"
                    run patchelf --set-rpath '$ORIGIN/../lib' "$(builder_install_prefix)/bin/rcc"
                    run patchelf --set-rpath '$ORIGIN/../lib' "$(builder_install_prefix)/bin/uic"
                    ;;
            esac
        ) || panic "Unable to change rpaths"

        # Copy binaries necessary for the build itself as well as static
        # libraries.
        copy_directory_files \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM-nowebengine" \
                bin/moc \
                bin/rcc \
                bin/uic \
                $QT_SHARED_LIBS

        case $SYSTEM in
            windows*)
                # libqtmainAndroidEmu.a is needed on Windows to implement WinMain et al.
                copy_directory_files \
                        "$(builder_install_prefix)" \
                        "$INSTALL_DIR/$SYSTEM" \
                        lib/libqtmainAndroidEmu.a
                ;;
            linux*)
                # Copy over libc++.so, so we can use it during build.
                cp $(aosp_clang_libcplusplus) "$INSTALL_DIR/$SYSTEM-nowebengine/lib"
                (cd "$INSTALL_DIR/$SYSTEM-nowebengine/lib" && ln -sf libc++.so libc++.so.1)
                ;;
        esac

        build_debug_info \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM-nowebengine" \
                $QT_SHARED_LIBS

        # Copy headers into common directory and add symlink
        copy_directory \
                "$(builder_install_prefix)"/include \
                "$INSTALL_DIR"/common/include.new

        directory_atomic_update \
                "$INSTALL_DIR"/common/include \
                "$INSTALL_DIR"/common/include.new

        (cd "$INSTALL_DIR/$SYSTEM-nowebengine" && rm -f include && ln -sf ../common/include include)

        # Move qconfig.h into its platform-specific directory now
        run mkdir -p "$INSTALL_DIR/$SYSTEM-nowebengine"/include.system/QtCore/
        run mv -f \
            "$INSTALL_DIR"/common/include/QtCore/qconfig.h \
            "$INSTALL_DIR/$SYSTEM-nowebengine"/include.system/QtCore/ \
            || panic "[$SYSTEM] Failed to move the platform-specific config file 'include/QtCore/qconfig.h'"
    ) || panic "[$SYSTEM] Could not build Qt!"

done

log "Done building Qt."
