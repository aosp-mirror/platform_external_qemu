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
shell_import utils/option_parser.shi

PROGRAM_PARAMETERS="<install-dir>"

PROGRAM_DESCRIPTION=\
"Generate a special toolchain wrapper that can be used to build binaries to be
included with the Android SDK. These binaries are guaranteed to run on systems
older than the build machine.

This is achieved by using the prebuilt toolchains found under:

  \$AOSP/prebuilts/gcc/<system>/host/

Where \$AOSP is the path to an AOSP checkout, and <system> corresponds to the
host system this script runs on (i.e. linux-x86_64 or darwin-x86_64). The toolchain
will be capable of handling C++11

If you have the 'ccache' program installed, the wrapper will use it
automatically unless you use the --no-ccache option.

The script will put under <install-dir>/ various tools, e.g.
'x86_64-linux-c++' for the 64-bit Linux compiler.

You can use the --print=<tool> option to print the corresponding name
or path, *instead* of creating a toolchain. Valid values for <tool> are:

   binprefix       -> Print the binprefix (e.g. 'x86_64-linux-')
   cc              -> Print the compiler name (e.g. 'x86_64_linux-cc')
   c++             -> Print the c++ compiler name.
   ld, ar, as, ... -> Same for other tools.
   sysroot         -> Print the path to directory corresponding to the current
                      toolchain.
   cflags          -> Print all the flags that are passed on to the c compiler.
   cxxflags        -> Print all the flags that are passed on to the c++ compiler.

Note: If you wish to cross compile to windows on darwin you will have to have a mingw
installation (brew install mingw-w64)
"

aosp_dir_register_option
aosp_register_clang_option

OPT_HOST=
option_register_var "--host=<system>" OPT_HOST "Host system to support."

OPT_BINPREFIX=
option_register_var "--binprefix=<prefix>" OPT_BINPREFIX "Specify toolchain binprefix [autodetected]"

OPT_PREFIX=
option_register_var "--prefix=<prefix>" OPT_PREFIX "Specify extra include/lib prefix."

OPT_PRINT=
option_register_var "--print=<tool>" OPT_PRINT "Print the exact name of a specific tool."

OPT_CCACHE=
option_register_var "--ccache=<program>" OPT_CCACHE "Use specific ccache program."

OPT_NO_CCACHE=
option_register_var "--no-ccache" OPT_NO_CCACHE "Don't try to probe and use ccache."

OPT_CXX11=
option_register_var "--cxx11" OPT_CXX11 "Enable C++11 features."

OPT_NOSTRIP=
option_register_var "--nostrip" OPT_NOSTRIP "Do not generate strip, needed for prebuilts only"

OPT_FORCE_FETCH_WINTOOLCHAIN=
option_register_var "--force-fetch-wintoolchain" OPT_FORCE_FETCH_WINTOOLCHAIN "Force fetch the Windows toolchain (google internal)"

OPT_VSDIR=/mnt/msvc/win8sdk
option_register_var "--vsdir=<msvc_install_dir>" OPT_VSDIR "Location of the msvc sdk. See documentation for details."


option_parse "$@"

if [ "$PARAMETER_COUNT" != 1 ]; then
    panic "This script requires a single parameter! See --help."
fi

INSTALL_DIR=$PARAMETER_1
# Determine host system type.
BUILD_HOST=$(get_build_os)
BUILD_ARCH=$(get_build_arch)
BUILD_BUILD_TARGET_TAG=${BUILD_HOST}-${BUILD_ARCH}
log2 "Found current build machine: $BUILD_BUILD_TARGET_TAG"

# Handle CCACHE related arguments.
CCACHE=
if [ "$OPT_CCACHE" ]; then
    if [ "$OPT_NO_CCACHE" ]; then
        panic "You cannot use both --ccache=<program> and --no-ccache at the same time."
    fi
    CCACHE=$(find_program "$OPT_CCACHE")
    if [ -z "$CCACHE" ]; then
        panic "Missing ccache program: $OPT_CCACHE"
    fi
elif [ -z "$OPT_NO_CCACHE" ]; then
    CCACHE=$(find_program ccache)
    if [ "$CCACHE" ]; then
        log2 "Auto-config: --ccache=$CCACHE"
    fi
fi

aosp_dir_parse_option

# Handle --host option.
if [ "$OPT_HOST" ]; then
    log "Detected OPT_HOST $OPT_HOST"
    case $OPT_HOST in
        linux-x86_64|linux-aarch64|darwin-x86_64|windows_msvc-x86_64)
            ;;
        *)
            panic "Invalid --host value: $OPT_HOST"
            ;;
    esac
    HOST=$OPT_HOST
    if [ "$OPT_HOST" = "linux-aarch64" ]; then
        log "Setting BUILD_ARCH to aarch64"
        BUILD_ARCH="aarch64"
    fi
else
    HOST=$BUILD_BUILD_TARGET_TAG
    log2 "Auto-config: --host=$HOST"
fi

# Handle --binprefix option.
BINPREFIX=
if [ "$OPT_BINPREFIX" ]; then
    # Allow a final - after the binprefix
    BINPREFIX=${OPT_BINPREFIX%%-}
    if [ "$BINPREFIX" ]; then
        BINPREFIX=${BINPREFIX}-
    fi
fi

# Generate a small toolchain wrapper program
#
# $1: program name, without any prefix (e.g. gcc, g++, ar, etc..)
# $2: source prefix (e.g. 'i586-mingw32msvc-')
# $3: destination prefix (e.g. 'i586-px-mingw32msvc-')
# $4: destination directory for the generated program
# $5: option, CLANG installation path.
#
# You may also define the following variables to pass extra tool flags:
#
#    EXTRA_CFLAGS
#    EXTRA_POSTCFLAGS
#    EXTRA_CXXFLAGS
#    EXTRA_POSTCXXFLAGS
#    EXTRA_LDFLAGS
#    EXTRA_POSTLDFLAGS
#    EXTRA_ARFLAGS
#    EXTRA_ASFLAGS
#    EXTRA_WINDRESFLAGS
#
# As well as a special variable containing commands to setup the
# environment before tool invocation:
#
#    EXTRA_ENV_SETUP
#
gen_wrapper_program ()
{
    local PROG="$1"
    local SRC_PREFIX="$2"
    local DST_PREFIX="$3"
    local DST_FILE="$4/${SRC_PREFIX}$PROG"
    local CLANG_BINDIR="$5"
    local FLAGS=""
    local POST_FLAGS=""
    local DST_PROG="$PROG"
    case $PROG in
      cc|gcc|cpp|clang)
          FLAGS=$FLAGS" $EXTRA_CFLAGS"
          POST_FLAGS=" $POST_CFLAGS"
          ;;
      c++|g++|clang++|cl)
          FLAGS=$FLAGS" $EXTRA_CXXFLAGS"
          POST_FLAGS=" $POST_CXXFLAGS"
          ;;
      ar|lib) FLAGS=$FLAGS" $EXTRA_ARFLAGS";;
      as) FLAGS=$FLAGS" $EXTRA_ASFLAGS";;
      ld|ld.bfd|ld.gold)
        FLAGS=$FLAGS" $EXTRA_LDFLAGS"
        POST_FLAGS=" $POST_LDFLAGS"
        ;;
      windres) FLAGS=$FLAGS" $EXTRA_WINDRESFLAGS";;
    esac

    # Redirect gcc -> clang if we are using clang.
    if [ "$CLANG_BINDIR" ]; then
        CLANG_BINDIR=${CLANG_BINDIR%/}
        case $PROG in
           cc|gcc|clang)
                DST_PROG=clang
                DST_PREFIX=$CLANG_BINDIR/
                ;;
            cl)
                DST_PROG=clang
                DST_PREFIX=$CLANG_BINDIR/
                ;;
            c++|g++|clang++)
                DST_PROG=clang++
                DST_PREFIX=$CLANG_BINDIR/
                ;;
            clang-tidy)
                DST_PREFIX=$CLANG_BINDIR/
                ;;
            ar|ranlib|nm|objcopy|objdump|strings|rc)
                DST_PROG=llvm-$PROG
                DST_PREFIX=$CLANG_BINDIR/
                ;;
            lib)
                DST_PROG=llvm-lib
                DST_PREFIX=$CLANG_BINDIR/
              ;;
            link)
                # The msvc compatible linker
                DST_PROG=clang-cl
                DST_PREFIX=$CLANG_BINDIR/
                FLAGS=$FLAGS" $EXTRA_CXXFLAGS"
                POST_FLAGS=" $POST_CXXFLAGS"
                ;;
        esac
    fi

    if [ -z "$DST_PREFIX" ]; then
        # Avoid infinite loop by getting real path of destination
        # program
        DST_PROG=$(which "$PROG" 2>/dev/null || true)
        if [ -z "$DST_PROG" ]; then
            log2 "Ignoring: ${SRC_PREFIX}$PROG"
            return
        fi
        DST_PREFIX=$(dirname "$DST_PROG")/
        DST_PROG=$(basename "$DST_PROG")
    fi

    if [ ! -f "${DST_PREFIX}$DST_PROG" ]; then
        case $DST_PROG in
            cc)
                # Our toolset doen't have separate cc binary for some platforms;
                # let those use 'gcc' directly.
                DST_PROG=gcc
                if [ ! -f "${DST_PREFIX}$DST_PROG" ]; then
                    log2 "  Skipping: ${SRC_PREFIX}$PROG  [missing destination program, ${DST_PREFIX}$DST_PROG]"
                    return
                fi
                ;;
            *)
                log2 "  Skipping: ${SRC_PREFIX}$PROG  [missing destination program, ${DST_PREFIX}$DST_PROG]"
                return
                ;;
         esac
    fi

    if [ "$CCACHE" ]; then
        DST_PREFIX="$CCACHE $DST_PREFIX"
    fi

    cat > "$DST_FILE" << EOF
#!/bin/sh
# Auto-generated by $(program_name), DO NOT EDIT!!
# Environment setup
$EXTRA_ENV_SETUP
# Tool invocation.
${DST_PREFIX}$DST_PROG $FLAGS "\$@" $POST_FLAGS

EOF
    chmod +x "$DST_FILE"
    log2 "  Generating: ${SRC_PREFIX}$PROG"
}

gen_dbg_splitter() {
    local SRC_PREFIX="$1"
    local DST_PREFIX="$2"
    local DST_FILE="$3/${SRC_PREFIX}strip"
    cat > "$DST_FILE" << EOF
#!/bin/sh
# Auto-generated by $(program_name), DO NOT EDIT!!
$EXTRA_ENV_SETUP
target=\$(basename \$1)
mkdir -p build/debug_info
${DST_PREFIX}objcopy --only-keep-debug  "\$1" "build/debug_info/\$target.debug"
${DST_PREFIX}objcopy --strip-unneeded  "\$1"
${DST_PREFIX}objcopy --add-gnu-debuglink="build/debug_info/\$target.debug" "\$1"

EOF
 chmod +x "$DST_FILE"
 log2 "  Generating: ${SRC_PREFIX}strip"
}

gen_dbg_splitter_darwin() {
    local SRC_PREFIX="$1"
    local DST_PREFIX="$2"
    local DST_FILE="$3/${SRC_PREFIX}strip"
    local CLANG_BINDIR="$4"

    cat > "$DST_FILE" << EOF
#!/bin/sh
# Auto-generated by $(program_name), DO NOT EDIT!!
# Environment setup
$EXTRA_ENV_SETUP
# Tool invocation.
target=\$(basename \$1)
$CLANG_BINDIR/dsymutil --out build/debug_info/\$target.dSYM \$1
EOF
 chmod +x "$DST_FILE"
  log2 "  Generating: ${SRC_PREFIX}strip"
}

# $1: source prefix
# $2: destination prefix
# $3: destination directory.
# $4: optional. Clang installation path.
gen_wrapper_toolchain () {
    local SRC_PREFIX="$1"
    local DST_PREFIX="$2"
    local DST_DIR="$3"
    local CLANG_BINDIR="$4"
    local PROG
    case "$CURRENT_HOST" in
        windows_msvc*)
          local COMPILERS="cc gcc clang c++ cl g++ clang++ cpp ld clang-tidy ar as ranlib strings nm objdump objcopy link lib rc"
          local PROGRAMS="dlltool"
          ;;
        linux-aarch64)
          local COMPILERS="cc gcc clang c++ g++ clang++ cpp ld clang-tidy"
          local PROGRAMS="as ar ranlib strings nm objdump objcopy aarch64-linux-gnu-dlltool"
          ;;
        *)
          local COMPILERS="cc gcc clang c++ g++ clang++ cpp ld clang-tidy"
          local PROGRAMS="as ar ranlib strings nm objdump objcopy dlltool"
    esac

    log "Generating toolchain wrappers in: $DST_DIR"
    silent_run mkdir -p "$DST_DIR"

    if [ -n "$SRC_PREFIX" ]; then
        SRC_PREFIX=${SRC_PREFIX%%-}-
    fi

    # We are doing clang on a posix system.. So lets
    # Symlink symbolizer, asan really wants this to be named llvm-symbolizer..
    ln -sf ${CLANG_BINDIR}/llvm-symbolizer ${DST_DIR}/llvm-symbolizer

    case "$CURRENT_HOST" in
        windows_msvc*)
            PROGRAMS="$PROGRAMS windres"
            ;;
    esac

    if [ "$CCACHE" ]; then
        # If this is clang, disable ccache-induced warnings and
        # restore colored diagnostics.
        # http://petereisentraut.blogspot.fr/2011/05/ccache-and-clang.html
        if (${DST_PREFIX}gcc --version 2>/dev/null | grep --color=never -q clang); then
            EXTRA_CLANG_FLAGS="-Qunused-arguments -fcolor-diagnostics"
            EXTRA_CFLAGS="$EXTRA_CFLAGS $EXTRA_CLANG_FLAGS"
            EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS $EXTRA_CLANG_FLAGS"
        fi
    fi

    for PROG in $COMPILERS; do
        gen_wrapper_program $PROG "$SRC_PREFIX" "$DST_PREFIX" "$DST_DIR" "$CLANG_BINDIR"
    done

    if [ ! -f "${DST_DIR}/c++" ]; then
        case "$CURRENT_HOST" in
            linux-aarch64*)
                ln -sf ${DST_DIR}/g++ ${DST_DIR}/c++
                ;;
        esac
    fi

    for PROG in $PROGRAMS; do
        gen_wrapper_program $PROG "$SRC_PREFIX" "$DST_PREFIX" "$DST_DIR"
    done

    # Setup additional host specific things
    if [ -z "$OPT_NOSTRIP" ]; then
      case "$CURRENT_HOST" in
        linux-x86_64)
          gen_dbg_splitter "$SRC_PREFIX" "$DST_PREFIX" "$DST_DIR"
          ;;
        darwin-x86_64)
          gen_dbg_splitter_darwin "$SRC_PREFIX" "$DST_PREFIX" "$DST_DIR" "$CLANG_BINDIR"
          ;;
      esac
    fi

    EXTRA_CFLAGS=
    EXTRA_CXXFLAGS=
    EXTRA_LDFLAGS=
    EXTRA_ARFLAGS=
    EXTRA_ASFLAGS=
    EXTRA_WINDRESFLAGS=
    EXTRA_ENV_SETUP=
}

please_install_proper_sdk_error() {
    log "No supported OSX SDKs/Xcode version found on the machine at $OSX_SDK_ROOT (Need any of: [$OSX_SDK_SUPPORTED])"
    log "You will need at least XCode version 10 to build!"
    log "Please obtain MacOSX$OSX_REQUIRED.sdk.tar.gz and make it available as an sdk."
    log "You can obtain this by tarring up the SDK directory from an XCode version that supports the SDK"
    log "See https://developer.apple.com/documentation/macos-release-notes for wich XCode release ships with the sdk you need."
    log "For example:"
    log "tar chvf MacOSX$OSX_REQUIRED.sdk.tar.gz /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/ MacOSX$OSX_REQUIRED.sdk"
    log "And making it available in your XCode (assuming it is in /Applications/Xcode.app):"
    log "tar xzvf ~/Downloads/MacOSX$OSX_REQUIRED.sdk.tar.gz -C /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$OSX_REQUIRED.sdk/"
    panic "Required sdk not available.."
}

# Configure the darwin toolchain.
prepare_build_for_darwin() {
    OSX_VERSION=$(sw_vers -productVersion)
    OSX_DEPLOYMENT_TARGET=10.11
    OSX_REQUIRED=10.13
    MIN_XCODE=10

    # This is the list of supported SDKs,
    OSX_SDK_SUPPORTED="${OSX_REQUIRED}"
    OSX_XCODE=$(xcodebuild -version | tr '\n' ' ')
    OSX_SDK_INSTALLED_LIST=$(xcodebuild -showsdks 2>/dev/null | \
            egrep --color=never -o " macosx\d+.\d+$" | sed -e "s/.*macosx10\.//g" | sort -n | \
            sed -e 's/^/10./g' | tr '\n' ' ')
    if [ -z "$OSX_SDK_INSTALLED_LIST" ]; then
        panic "Please install XCode on this machine!"
    fi
    log "OSX: Installed SDKs: $OSX_SDK_INSTALLED_LIST"
    for supported_sdk in $(echo "$OSX_SDK_SUPPORTED" | tr ' ' '\n' | sort -r)
    do
        POSSIBLE_OSX_SDK_VERSION=$(echo "$OSX_SDK_INSTALLED_LIST" | tr ' ' '\n' | grep $supported_sdk | head -1)
        if [ -n "$POSSIBLE_OSX_SDK_VERSION" ]; then
            OSX_SDK_VERSION=$POSSIBLE_OSX_SDK_VERSION
        fi
    done

    # Extract the Xcode version, and use version sort to stack the installed version
    # on top of what you have. If your version is to low it ends up on top.
    OSX_XCODE_VERSION=$(xcodebuild -version | egrep 'Xcode (\d+.\d+)' | sed 's/Xcode //g')
    VERSION_SORT=$(printf "$MIN_XCODE\n$OSX_XCODE_VERSION" | sort --version-sort | head -n 1)

    if test "$VERSION_SORT" != "$MIN_XCODE"; then
        log "You need to have at least XCode 10 installed, not ${OSX_XCODE}"
        please_install_proper_sdk_error
    fi

    XCODE_PATH=$(xcode-select -print-path 2>/dev/null)

    log "OSX: Using ${OSX_XCODE} with SDK version $OSX_SDK_VERSION"
    log "OSX: XCode path: $XCODE_PATH"

    if [ -z "$OSX_SDK_VERSION" ]; then
        please_install_proper_sdk_error
    fi

    OSX_SDK_ROOT=$XCODE_PATH/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk
    log2 "OSX: Looking for $OSX_SDK_ROOT"
    if [ ! -d "$OSX_SDK_ROOT" ]; then
        OSX_SDK_ROOT=/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk
        log2 "OSX: Looking for $OSX_SDK_ROOT"
        if [ ! -d "$OSX_SDK_ROOT" ]; then
            please_install_proper_sdk_error
        fi
    fi
    log "OSX: Using SDK at $OSX_SDK_ROOT"
    EXTRA_ENV_SETUP="export SDKROOT=$OSX_SDK_ROOT"
    CLANG_BINDIR=$PREBUILT_TOOLCHAIN_DIR/bin
    PREBUILT_TOOLCHAIN_DIR=

    GNU_CONFIG_HOST=

    common_FLAGS="-target x86_64-apple-darwin12.0.0"
    var_append common_FLAGS " -isysroot $OSX_SDK_ROOT"
    var_append common_FLAGS " -mmacosx-version-min=$OSX_DEPLOYMENT_TARGET"
    var_append common_FLAGS " -DMACOSX_DEPLOYMENT_TARGET=$OSX_DEPLOYMENT_TARGET"
    EXTRA_CFLAGS="$common_FLAGS -B/usr/bin"
    EXTRA_CXXFLAGS="$common_FLAGS -B/usr/bin"
    var_append EXTRA_CXXFLAGS "-stdlib=libc++"
    EXTRA_LDFLAGS="$common_FLAGS"
    DST_PREFIX=
}

prepare_build_for_linux_x86_64() {
    GCC_DIR="${PREBUILT_TOOLCHAIN_DIR}/lib/gcc/x86_64-linux/4.8.3/"
    CLANG_BINDIR=$AOSP_DIR/$(aosp_prebuilt_clang_dir_for linux)
    CLANG_DIR=$(realpath $CLANG_BINDIR/..)
    GNU_CONFIG_HOST=x86_64-linux
    CLANG_VERSION=$(get_clang_version)
    SYSROOT="${PREBUILT_TOOLCHAIN_DIR}/sysroot"

    # Clang will invoke the linker, but will not correctly infer the
    # -gcc-toolchain, so we have to manually configure the various paths.
    # Note that this can result in warnings of unused flags if we are not
    # linking and merely translating a .c or .cpp file
    local GCC_LINK_FLAGS=
    var_append GCC_LINK_FLAGS "--gcc-toolchain=$PREBUILT_TOOLCHAIN_DIR"
    var_append GCC_LINK_FLAGS "-B$PREBUILT_TOOLCHAIN_DIR/lib/gcc/x86_64-linux/4.8.3/"
    var_append GCC_LINK_FLAGS "-L$PREBUILT_TOOLCHAIN_DIR/lib/gcc/x86_64-linux/4.8.3/"
    var_append GCC_LINK_FLAGS "-L$PREBUILT_TOOLCHAIN_DIR/x86_64-linux/lib64/"
    var_append GCC_LINK_FLAGS "-fuse-ld=${PREBUILT_TOOLCHAIN_DIR}/bin/x86_64-linux-ld"
    var_append GCC_LINK_FLAGS "-L${CLANG_BINDIR}/../lib64/clang/${CLANG_VERSION}/lib/linux/"
    var_append GCC_LINK_FLAGS "-L${CLANG_BINDIR}/../lib64/clang/${CLANG_VERSION}/include"
    var_append GCC_LINK_FLAGS "-L${CLANG_BINDIR}/../lib64/"
    var_append GCC_LINK_FLAGS "--sysroot=${SYSROOT}"

    if [ $(get_verbosity) -gt 3 ]; then
      # This will get pretty crazy, but useful if you want to debug linker issues.
      var_append GCC_LINK_FLAGS "-Wl,-verbose"
      var_append GCC_LINK_FLAGS "-v"
    else
      # You're likely to always hit this due to our linker workarounds..
      var_append GCC_LINK_FLAGS "-Wno-unused-command-line-argument"
    fi


    EXTRA_CFLAGS="-m64"
    # var_append EXTRA_CFLAGS "-std=c89"
    var_append EXTRA_CFLAGS "-isystem $SYSROOT/usr/include"
    var_append EXTRA_CFLAGS "-isystem $SYSROOT/usr/include/x86_64-linux-gnu"
    var_append EXTRA_CFLAGS ${GCC_LINK_FLAGS}


    EXTRA_CXXFLAGS="-m64"
    var_append EXTRA_CXXFLAGS "-stdlib=libc++"
    var_append EXTRA_CXXFLAGS ${GCC_LINK_FLAGS}

    # Make sure we can find libc++
    EXTRA_LDFLAGS="-m64"
    var_append EXTRA_LDFLAGS "-L${CLANG_BINDIR}/../lib64/clang/${CLANG_VERSION}/lib/linux/"
    var_append EXTRA_LDFLAGS "-L${CLANG_BINDIR}/../lib64/"
    # Make sure we don't accidently pick up libstdc++
    var_append EXTRA_LDFLAGS "-nodefaultlibs"

    # If we manually call the linker we need to pass in the correct default libs
    # we link against. These have to go last!
    POST_LDFLAGS=
    var_append POST_LDFLAGS "-lc"
    var_append POST_LDFLAGS "-lc++"
    var_append POST_LDFLAGS "-lm"
    var_append POST_LDFLAGS "-lgcc"
    var_append POST_LDFLAGS "-lgcc_s"
}

prepare_build_for_linux_aarch64() {
    GCC_DIR="/usr/"
    # aarch64 cross compile is not present in host aosp toolchain, use
    # system cross compiler /usr/bin/aarch64-linux-gnu-*
    CLANG_BINDIR=
    CLANG_DIR=
    GNU_CONFIG_HOST=
    CLANG_VERSION=
    SYSROOT="/"

    # Clang will invoke the linker, but will not correctly infer the
    # -gcc-toolchain, so we have to manually configure the various paths.
    # Note that this can result in warnings of unused flags if we are not
    # linking and merely translating a .c or .cpp file
    local GCC_LINK_FLAGS=

    if [ $(get_verbosity) -gt 3 ]; then
      # This will get pretty crazy, but useful if you want to debug linker issues.
      var_append GCC_LINK_FLAGS "-Wl,-verbose"
      var_append GCC_LINK_FLAGS "-v"
    else
      var_append GCC_LINK_FLAGS ""
    fi


    EXTRA_CFLAGS=
    var_append EXTRA_CFLAGS ${GCC_LINK_FLAGS}


    EXTRA_CXXFLAGS=
    var_append EXTRA_CXXFLAGS ${GCC_LINK_FLAGS}

    # Make sure we can find libgcc on aarch64
    EXTRA_LDFLAGS="-L/usr/lib/gcc/aarch64-linux-gnu/5"

    # If we manually call the linker we need to pass in the correct default libs
    # we link against. These have to go last!
    POST_LDFLAGS=
    var_append POST_LDFLAGS "-lc"
    var_append POST_LDFLAGS "-lc++"
    var_append POST_LDFLAGS "-lm"
    var_append POST_LDFLAGS "-lgcc"
    var_append POST_LDFLAGS "-lgcc_s"
    var_append POST_LDFLAGS "-ldl"

    # aarch64 use host toolchain.  no prefix needed.
    DST_PREFIX=/usr/bin/aarch64-linux-gnu-
}

configure_google_storage() {
    local DEPOT_TOOLS=$(aosp_depot_tools_dir)
    ${DEPOT_TOOLS}/download_from_google_storage --config
}

fetch_dependencies_msvc() {
    # Takes two parameters:
    # $1: The path to the mount point for ciopfs,
    # $2: The path to the data directory for ciopfs

    # if you really want to force pull-down the sdk again, you can specify it in the rebuild script via
    # --force-refresh-winsdk
    CLANG_VERSION=$(get_clang_version)
    # The mount point for ciopfs (case-insensitive)
    local MOUNT_POINT="$1"
    # Where the windows sdk is actually stored in ciopfs
    local DATA_POINT="$2"

    if [ -d "$MOUNT_POINT" ] && [ -d "$DATA_POINT" ]; then
      # We will cache the windows sdk and reuse it for each build, unless you specify to force pull it down
      # via --force-fetch-wintoolchain
      if [ "$OPT_FORCE_FETCH_WINTOOLCHAIN" ]; then
        # Need to unmount $MOUNT_POINT first or we can't delete it
        run fusermount -u ${MOUNT_POINT}
        run rm -rf $MOUNT_POINT
        run rm -rf $DATA_POINT
      else
        return 0
      fi
    fi

    # Create the case-insensitive filesystem
    run mkdir -p {$MOUNT_POINT,$DATA_POINT}
    local CIOPFS="${AOSP_DIR}/prebuilts/android-emulator-build/common/ciopfs/linux-x86_64/ciopfs"
    run $CIOPFS -o use_ino $DATA_POINT $MOUNT_POINT

    # Download the windows sdk
    local DEPOT_TOOLS=$(aosp_depot_tools_dir)
    local MSVC_HASH=$(aosp_msvc_hash)

    log "Looking to get tools version: ${MSVC_HASH}"
    log "Downloading the SDK (this might take a couple of minutes ...)"
    run ${DEPOT_TOOLS}/win_toolchain/get_toolchain_if_necessary.py --force --toolchain-dir=$MOUNT_POINT $MSVC_HASH
}

get_clang_version() {
    CLANG_BINDIR=$AOSP_DIR/$(aosp_prebuilt_clang_dir_for ${BUILD_HOST})
    CLANG_DIR=$(realpath $CLANG_BINDIR/..)
    if [ "$BUILD_HOST" = "darwin" ]; then
       CLANG_VERSION=$(${CLANG_BINDIR}/clang -v 2>&1 | egrep -E 'clang version \d+.\d+.\d' -o | egrep -E '\d+.\d+.\d+' -o)
    else
       CLANG_VERSION=$(${CLANG_BINDIR}/clang -v 2>&1 | grep -P 'clang version \d+.\d+.\d' -o | grep -P '\d+.\d+.\d+' -o)
    fi
    echo $CLANG_VERSION
}

prepare_build_for_windows_msvc() {
    CLANG_BINDIR=$AOSP_DIR/$(aosp_prebuilt_clang_dir_for ${BUILD_HOST})
    CLANG_DIR=$(realpath $CLANG_BINDIR/..)
    CLANG_VERSION=$(get_clang_version)

    # Let's cache the Windows SDK toolchain in the prebuilts/android-emulator-build/msvc
    # fetch_dependencies_msvc will pull down the sdk from google storage if the provided directories are
    # not found.
    MSVC_ROOT_DIR="${AOSP_DIR}/prebuilts/android-emulator-build/msvc"
    CIOPFS_DIR="${AOSP_DIR}/prebuilts/android-emulator-build/.ciopfs"

    # Check to see if we need to fetch the toolchain..
    if [ -d "$OPT_VSDIR" ]; then
      MSVC_DIR=$OPT_VSDIR
    else
      log "Configuring msvc.."
      MSVC_HASH=$(aosp_msvc_hash)
      MSVC_DIR=${MSVC_ROOT_DIR}/vs_files/${MSVC_HASH}
      fetch_dependencies_msvc "${MSVC_ROOT_DIR}" "${CIOPFS_DIR}"
    fi

    # We cannot have multiple SDK versions in the same dir
    MSVC_VER=$(ls -1 ${MSVC_DIR}/win_sdk/Include)
    log "Using version [${MSVC_VER}]"

    VC_VER=$(ls -1 ${MSVC_DIR}/VC/Tools/MSVC)
    log "Using visual studio version: ${VC_VER}"

    run mkdir -p $MSVC_DIR/clang-intrins/${CLANG_VERSION}/include
    run rsync -r ${CLANG_DIR}/lib64/clang/${CLANG_VERSION}/include  $MSVC_DIR/clang-intrins/${CLANG_VERSION}

    local CLANG_LINK_FLAGS=
    var_append CLANG_LINK_FLAGS "-fuse-ld=lld"
    var_append CLANG_LINK_FLAGS "-L${MSVC_DIR}/VC/Tools/MSVC/${VC_VER}/lib/x64"
    var_append CLANG_LINK_FLAGS "-L${MSVC_DIR}/VC/Tools/MSVC/${VC_VER}/atlmfc/lib/x64"
    var_append CLANG_LINK_FLAGS "-L${MSVC_DIR}/win_sdk/lib/${MSVC_VER}/ucrt/x64"
    var_append CLANG_LINK_FLAGS "-L${MSVC_DIR}/win_sdk/lib/${MSVC_VER}/um/x64"
    var_append CLANG_LINK_FLAGS "\"-L${MSVC_DIR}/DIA SDK/lib/amd64\""
    var_append CLANG_LINK_FLAGS "-Wl,-nodefaultlib:libcmt,-defaultlib:msvcrt,-defaultlib:oldnames"
    # Other linker flags to make cross-compiling upstream-qemu work without adding any additional
    # flags to it's makefile.
    var_append CLANG_LINK_FLAGS "-lshell32 -luser32 -ladvapi32 -liphlpapi -lwldap32 -lmfuuid -lwinmm"

    if [ $(get_verbosity) -gt 3 ]; then
      # This will get pretty crazy, but useful if you want to debug linker issues.
      var_append CLANG_LINK_FLAGS "-Wl,-verbose"
      var_append CLANG_LINK_FLAGS "-v"
    else
      # You're likely to always hit this due to our linker workarounds..
      var_append CLANG_LINK_FLAGS "-Wno-unused-command-line-argument"
    fi


    EXTRA_CFLAGS="-target x86_64-pc-win32"
    var_append EXTRA_CFLAGS "-DLIEF_DISABLE_FROZEN=on"
    var_append EXTRA_CFLAGS "-D_MD"
    var_append EXTRA_CFLAGS "-D_DLL"
    var_append EXTRA_CFLAGS "-D_AMD64_"
    var_append EXTRA_CFLAGS "-DWIN32_LEAN_AND_MEAN"
    var_append EXTRA_CFLAGS "-DNOMINMAX"
    var_append EXTRA_CFLAGS "-D_CRT_SECURE_NO_WARNINGS"
    var_append EXTRA_CFLAGS "-D_USE_MATH_DEFINES"
    # Target Win 7 >
    var_append EXTRA_CFLAGS "-D_WIN32_WINNT=0x0601"
    var_append EXTRA_CFLAGS "-D_WINVER=0x0601"
    var_append EXTRA_CFLAGS "-Wno-msvc-not-found"
    var_append EXTRA_CFLAGS "-Wno-address-of-packed-member"
    var_append EXTRA_CFLAGS "-Wno-incompatible-ms-struct"
    var_append EXTRA_CFLAGS "-Wno-c++11-narrowing"
    # Disable builtin #include directories
    var_append EXTRA_CFLAGS "-nobuiltininc"
    var_append EXTRA_CFLAGS "-isystem $MSVC_DIR/clang-intrins/${CLANG_VERSION}/include"
    var_append EXTRA_CFLAGS "-isystem $MSVC_DIR/VC/Tools/MSVC/${VC_VER}/include"
    var_append EXTRA_CFLAGS "-isystem $MSVC_DIR/win_sdk/include/${MSVC_VER}/ucrt"
    var_append EXTRA_CFLAGS "-isystem $MSVC_DIR/win_sdk/include/${MSVC_VER}/um"
    var_append EXTRA_CFLAGS "-isystem $MSVC_DIR/win_sdk/include/${MSVC_VER}/winrt"
    var_append EXTRA_CFLAGS "-isystem $MSVC_DIR/win_sdk/include/${MSVC_VER}/shared"
    var_append EXTRA_CFLAGS "-isystem \"$MSVC_DIR/DIA SDK/include\""


    var_append EXTRA_CFLAGS ${CLANG_LINK_FLAGS}


    EXTRA_CXXFLAGS="-target x86_64-pc-win32"
    var_append EXTRA_CXXFLAGS "-DLIEF_DISABLE_FROZEN=on"
    var_append EXTRA_CXXFLAGS "-D_MD"
    var_append EXTRA_CXXFLAGS "-D_DLL"
    var_append EXTRA_CXXFLAGS "-D_AMD64_"
    var_append EXTRA_CXXFLAGS "-DWIN32_LEAN_AND_MEAN"
    var_append EXTRA_CXXFLAGS "-DNOMINMAX"
    # Target Win 7 >
    var_append EXTRA_CXXFLAGS "-D_WIN32_WINNT=0x0601"
    var_append EXTRA_CXXFLAGS "-D_WINVER=0x0601"
    var_append EXTRA_CXXFLAGS "-D_CRT_SECURE_NO_WARNINGS"
    var_append EXTRA_CXXFLAGS "-D_USE_MATH_DEFINES"
    var_append EXTRA_CXXFLAGS "-Wno-msvc-not-found"
    var_append EXTRA_CXXFLAGS "-Wno-address-of-packed-member"
    var_append EXTRA_CXXFLAGS "-Wno-incompatible-ms-struct"
    var_append EXTRA_CXXFLAGS "-Wno-c++11-narrowing"
    var_append EXTRA_CXXFLAGS "-fms-compatibility"
    var_append EXTRA_CXXFLAGS "-fdelayed-template-parsing"
    var_append EXTRA_CXXFLAGS "-fms-extensions"
    # Disable builtin #include directories
    var_append EXTRA_CXXFLAGS "-nobuiltininc"
    # Disable standard #include directories for the C++ standard library
    var_append EXTRA_CXXFLAGS "-nostdinc++"
    var_append EXTRA_CXXFLAGS "-isystem $MSVC_DIR/clang-intrins/${CLANG_VERSION}/include"
    var_append EXTRA_CXXFLAGS "-isystem $MSVC_DIR/VC/Tools/MSVC/${VC_VER}/include"
    var_append EXTRA_CXXFLAGS "-isystem $MSVC_DIR/VC/Tools/MSVC/${VC_VER}/atlmfc/include"
    var_append EXTRA_CXXFLAGS "-isystem $MSVC_DIR/win_sdk/include/${MSVC_VER}/ucrt"
    var_append EXTRA_CXXFLAGS "-isystem $MSVC_DIR/win_sdk/include/${MSVC_VER}/um"
    var_append EXTRA_CXXFLAGS "-isystem $MSVC_DIR/win_sdk/include/${MSVC_VER}/winrt"
    var_append EXTRA_CXXFLAGS "-isystem $MSVC_DIR/win_sdk/include/${MSVC_VER}/shared"
    var_append EXTRA_CXXFLAGS "-isystem \"$MSVC_DIR/DIA SDK/include\""
    var_append EXTRA_CXXFLAGS ${CLANG_LINK_FLAGS}

    EXTRA_LDFLAGS="-target x86_64-pc-win32"
    # Make sure we don't accidently pick up any clang libs or msvc libs
    # that are included by default (libcmt).
    var_append EXTRA_LDFLAGS "-nodefaultlibs"
}


# Prints info based on the passed in parameter
# $1 The information we want to print, from { binprefix, sysroot, clang-dir,
# clang-version}
print_info () {
    local PRINT=$1

    # If $BINPREFIX is not empty, ensure it has a trailing -.
    if [ "$BINPREFIX" ]; then
        BINPREFIX=${BINPREFIX%%-}-
    fi
    case $PRINT in
        binprefix)
            if [ "$BUILD_HOST" = "darwin" ]; then
                BINPREFIX=""
            fi
            printf "%s\n" "$BINPREFIX"
            ;;
        sysroot)
            local SUBDIR
            SUBDIR="$(aosp_prebuilt_toolchain_sysroot_subdir_for "${CURRENT_HOST}")"
            if [ -n "${SUBDIR}" ]; then
                printf "${AOSP_DIR}/${SUBDIR}"
            fi
            ;;
        cflags)
          echo $EXTRA_CFLAGS
          ;;
        cxxflags)
          echo $EXTRA_CXXFLAGS
          ;;
        clang-dir)
          printf "%s\n" "${CLANG_DIR}"
            ;;
        clang-version)
            CLANG_VERSION=$(${CLANG_BINDIR}/clang -v 2>&1 | head -n 1 | awk '{print $4}')
            printf "%s\n" "$CLANG_VERSION"
            ;;
        inccplusplus)
            echo "${CLANG_DIR}/include/c++/v1"
            ;;
        libcplusplus)
            if [ "$BUILD_HOST" = "linux" -a "$BUILD_ARCH" = "aarch64" ]; then
                printf "%s\n" "/usr/aarch64-linux-gnu/lib/libstdc++.so.6"
            else
                printf "%s\n" "$CLANG_DIR/lib64/libc++.so"
            fi
            ;;
        libasan-dir)
            printf "%s" "$CLANG_DIR/lib64/clang/5.0/lib/linux/"
            ;;
        *)
            printf "%s\n" "${BINPREFIX}$PRINT"
            ;;
    esac
}


# Prepare the build for a given host system.
# $1: Host system tag (e.g. linux-x86_64)
prepare_build_for_host () {
    local CURRENT_HOST=$1
    EXTRA_ENV_SETUP=
    CLANG_BINDIR=
    PREBUILT_TOOLCHAIN_SUBDIR=$(aosp_prebuilt_toolchain_subdir_for $CURRENT_HOST)
    PREBUILT_TOOLCHAIN_DIR=$AOSP_DIR/$PREBUILT_TOOLCHAIN_SUBDIR
    TOOLCHAIN_PREFIX=$(aosp_prebuilt_toolchain_prefix_for $CURRENT_HOST)
    DST_PREFIX=$PREBUILT_TOOLCHAIN_DIR/bin/$TOOLCHAIN_PREFIX

    case $CURRENT_HOST in
        linux-*)
            prepare_build_for_linux_$BUILD_ARCH
            ;;
        darwin-*)
            prepare_build_for_darwin
            ;;
        windows_msvc-*)
            if [ "$BUILD_HOST" = "darwin" ]; then
                which x86_64-w64-mingw32-gcc > /dev/null || panic "No mingw install available!"
                TOOLCHAIN_PREFIX=x86_64-w64-mingw32-
                DST_PREFIX=$(dirname $(which x86_64-w64-mingw32-gcc))
                DST_PREFIX=${DST_PREFIX}/$TOOLCHAIN_PREFIX
            fi
            prepare_build_for_windows_msvc
            ;;
    esac


    log2 "Exited prepare build. compiler bindir: $CLANG_BINDIR"

    if [ "$OPT_BINPREFIX" ]; then
        BINPREFIX=${OPT_BINPREFIX%%-}
    else
        BINPREFIX=$GNU_CONFIG_HOST
    fi

    if [ "$OPT_PREFIX" ]; then
        var_append EXTRA_CFLAGS "-I$OPT_PREFIX/include"
        var_append EXTRA_CXXFLAGS "-I$OPT_PREFIX/include"
        var_append EXTRA_LDFLAGS "-L$OPT_PREFIX/lib"
    fi
    if [ "$OPT_PRINT" ]; then
        print_info $OPT_PRINT
    elif [ "$OPT_FORCE_FETCH_WINTOOLCHAIN" ]; then
        # The windows toolchain should have been refreshed in prepare_build_for_windows_msvc
        # so don't need to do anything here.
        return 0
    else
        log2 "Generating ${BINPREFIX%%-} wrapper toolchain in $INSTALL_DIR"
        gen_wrapper_toolchain "$BINPREFIX" "$DST_PREFIX" "$INSTALL_DIR" "$CLANG_BINDIR"

        # Create pkgconfig link for other scripts.
        case $CURRENT_HOST in
            linux-x86*)
                ln -sfn "$PREBUILT_TOOLCHAIN_DIR/sysroot/usr/lib/pkgconfig" "$INSTALL_DIR/pkgconfig"
                ;;
        esac
    fi
}
prepare_build_for_host $HOST
