#!/bin/bash
#
# this script is used to rebuild all QEMU binaries for the host
# platforms.
#
# assume that the device tree is in TOP
#

set -e
export LANG=C
export LC_ALL=C

PROGDIR=$(dirname "$0")
VERBOSE=1

BUILD_QEMU_ANDROID=
MINGW=
NO_TESTS=
OUT_DIR=objs

for OPT; do
    case $OPT in
        --aosp-prebuilts-dir=*)
            ANDROID_EMULATOR_PREBUILTS_DIR=${OPT##--aosp-prebuilts-dir=}
            ;;
        --build-qemu-android)
            BUILD_QEMU_ANDROID=true
            ;;
        --mingw)
            MINGW=true
            ;;
        --verbose)
            VERBOSE=$(( $VERBOSE + 1 ))
            ;;
        --verbosity=*)
            VERBOSE=${OPT##--verbosity=}
            ;;
        --no-tests)
            NO_TESTS=true
            ;;
        --out-dir=*)
            OUT_DIR=${OPT##--out-dir=}
            ;;
        --help|-?)
            VERBOSE=2
            ;;
    esac
done

panic () {
    echo "ERROR: $@"
    exit 1
}

run () {
    if [ "$VERBOSE" -gt 2 ]; then
        echo "COMMAND: $@"
        "$@"
    elif [ "$VERBOSE" -gt 1 ]; then
        "$@"
    else
        "$@" >/dev/null 2>&1
    fi
}

log () {
    if [ "$VERBOSE" -gt 1 ]; then
        printf "%s\n" "$*"
    fi
}

HOST_OS=$(uname -s)
case $HOST_OS in
    Linux)
        HOST_NUM_CPUS=`cat /proc/cpuinfo | grep processor | wc -l`
        ;;
    Darwin|FreeBSD)
        HOST_NUM_CPUS=`sysctl -n hw.ncpu`
        ;;
    CYGWIN*|*_NT-*)
        HOST_NUM_CPUS=$NUMBER_OF_PROCESSORS
        ;;
    *)  # let's play safe here
        HOST_NUM_CPUS=1
esac

case $HOST_OS in
    Linux)
        HOST_SYSTEM=linux
        ;;
    Darwin)
        HOST_SYSTEM=darwin
        ;;
    FreeBSD)
        HOST_SYSTEM=freebsd
        ;;
    CYGWIN*|*_NT-*)
        HOST_SYSTEM=windows
        ;;
    *)
        panic "Host system is not supported: $HOST_OS"
esac

# Return the type of a given file, using the /usr/bin/file command.
# $1: executable path.
# Out: file type string, or empty if the path is wrong.
get_file_type () {
    /usr/bin/file "$1" 2>/dev/null
}

# Return true iff the file type string |$1| contains the expected
# substring |$2|.
# $1: executable file type
# $2: expected file type substring
check_file_type_substring () {
    printf "%s\n" "$1" | grep -q -E -e "$2"
}

# Copy a single file
# $1: Source file path.
# $2: Destination file path (not a directory!)
copy_file () {
    local SRC_FILE="$1"
    local DST_FILE="$2"

    if [ "$VERBOSE" = "1" ]; then
        log "Copying $SRC_FILE -> $DST_FILE"
    fi

    case $DST_FILE in
        */)
            if [ ! -d "$DST_FILE" ]; then
                run mkdir -p "${DST_FILE%%/}" ||
                        panic "Cannot create destination directory: $DST_FILE"
            fi
            DST_FILE=${DST_FILE%%/}
            ;;
        *)
            if [ -d "$DST_FILE" ]; then
                DST_FILE=${DST_FILE}/
            else
                local DST_DIR="$(dirname "$DST_FILE")"
                if [ ! -d "$DST_DIR" ]; then
                    run mkdir -p "$(dirname "$DST_FILE")" ||
                            panic "Could not create destination directory: $DST_DIR"
                fi
            fi
            ;;
    esac

    run cp -a "$SRC_FILE" "$DST_FILE" ||
            panic "Could not copy $SRC_FILE into $DST_FILE !?"
}

# Define EXPECTED_32BIT_FILE_TYPE and EXPECTED_64BIT_FILE_TYPE depending
# on the current target platform. Then EXPECTED_EMULATOR_BITNESS and
# EXPECTED_EMULATOR_FILE_TYPE accordingly.
if [ "$MINGW" ]; then
    EXPECTED_32BIT_FILE_TYPE="PE32 executable \(console\) Intel 80386"
    EXPECTED_64BIT_FILE_TYPE="PE32\+ executable \(console\) x86-64"
    EXPECTED_EMULATOR_BITNESS=32
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_32BIT_FILE_TYPE
    TARGET_OS=windows
elif [ "$HOST_OS" = "Darwin" ]; then
    EXPECTED_32BIT_FILE_TYPE="Mach-O executable i386"
    EXPECTED_64BIT_FILE_TYPE="Mach-O 64-bit executable x86_64"
    EXPECTED_EMULATOR_BITNESS=64
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_64BIT_FILE_TYPE
    TARGET_OS=darwin
else
    EXPECTED_32BIT_FILE_TYPE="ELF 32-bit LSB +executable, Intel 80386"
    EXPECTED_64BIT_FILE_TYPE="ELF 32-bit LSB +executable, x86-64"
    EXPECTED_EMULATOR_BITNESS=32
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_32BIT_FILE_TYPE
    TARGET_OS=linux
fi

# Build the binaries from sources.
cd "$PROGDIR"
rm -rf objs
echo "Configuring build."
export IN_ANDROID_REBUILD_SH=1
run ./android-configure.sh --out-dir=$OUT_DIR "$@" ||
    panic "Configuration error, please run ./android-configure.sh to see why."

echo "Building sources."
run make -j$HOST_NUM_CPUS OBJS_DIR="$OUT_DIR" ||
    panic "Could not build sources, please run 'make' to see why."

if [ "$BUILD_QEMU_ANDROID" ]; then
    # Rebuild qemu-android binaries.
    echo "Building qemu-android binaries."
    unset ANDROID_EMULATOR_DARWIN_SSH &&
    $PROGDIR/android/scripts/build-qemu-android.sh \
        --verbosity=$VERBOSE \
        --host=$HOST_SYSTEM-x86_64 \
        --force
fi

# Copy qemu-android binaries, if any.
PREBUILTS_DIR=$ANDROID_EMULATOR_PREBUILTS_DIR
if [ -z "$PREBUILTS_DIR" ]; then
    PREBUILTS_DIR=$PROGDIR/../../prebuilts/android-emulator-build
else
    if [ -d "$PREBUILTS_DIR"/android-emulator-build ]; then
        PREBUILTS_DIR=$PREBUILTS_DIR/android-emulator-build
    fi
fi
QEMU_ANDROID_HOSTS=
QEMU_ANDROID_BINARIES=
if [ -d "$PREBUILTS_DIR/qemu-android" ]; then
    HOST_PREFIX=$TARGET_OS
    if [ "$HOST_PREFIX" ]; then
        QEMU_ANDROID_BINARIES=$(cd "$PREBUILTS_DIR"/qemu-android && ls $HOST_PREFIX-*/qemu-system-* 2>/dev/null || true)
    fi
fi
if [ -z "$QEMU_ANDROID_BINARIES" ]; then
    echo "WARNING: Missing qemu-android prebuilts. Please run build-qemu-android.sh!"
else
    echo "Copying prebuilt $HOST_PREFIX qemu-android binaries to $OUT_DIR/qemu/"
    for QEMU_ANDROID_BINARY in $QEMU_ANDROID_BINARIES; do
        copy_file "$PREBUILTS_DIR"/qemu-android/$QEMU_ANDROID_BINARY \
                $OUT_DIR/qemu/$QEMU_ANDROID_BINARY
    done
fi

if [ -d "$PREBUILTS_DIR/mesa" ]; then
    MESA_HOST=$HOST_SYSTEM
    if [ "$MINGW" ]; then
        MESA_HOST=windows
    fi
    case $MESA_HOST in
        windows)
            MESA_LIBNAME=opengl32.dll
            ;;
        linux)
            MESA_LIBNAME=libGL.so
            ;;
        *)
            MESA_LIBNAME=
    esac
    for LIBNAME in $MESA_LIBNAME; do
        for MESA_ARCH in x86 x86_64; do
            if [ "$MESA_ARCH" = "x86" ]; then
                MESA_LIBDIR=lib
            else
                MESA_LIBDIR=lib64
            fi
            MESA_LIBRARY=$(ls "$PREBUILTS_DIR/mesa/$MESA_HOST-$MESA_ARCH/lib/$LIBNAME" 2>/dev/null || true)
            if [ "$MESA_LIBRARY" ]; then
                MESA_DSTDIR="$OUT_DIR/$MESA_LIBDIR/gles_mesa"
                MESA_DSTLIB="$LIBNAME"
                if [ "$LIBNAME" = "opengl32.dll" ]; then
                    MESA_DSTLIB="mesa_opengl32.dll"
                fi
                echo "Copying $MESA_HOST-$MESA_ARCH $LIBNAME library to $MESA_DSTDIR"
                copy_file "$MESA_LIBRARY" "$MESA_DSTDIR/$MESA_DSTLIB"
                if [ "$MESA_HOST" = "linux" -a "$LIBNAME" = "libGL.so" ]; then
                    # Special handling for Linux, this is needed because SDL
                    # will actually try to load libGL.so.1 before GPU emulation
                    # is initialized. This is actually a link to the system's
                    # libGL.so, and will thus prevent the Mesa version from
                    # loading. By creating the symlink, Mesa will also be used
                    # by SDL.
                    run ln -sf libGL.so "$MESA_DSTDIR/libGL.so.1"
                fi
            fi
        done
    done
fi

CONFIG_MAKE=$OUT_DIR/build/config.make
if [ ! -f "$CONFIG_MAKE" ]; then
    panic "Cannot find: $CONFIG_MAKE"
fi

# Copy Qt shared libraries, if needed.
EMULATOR_USE_QT=$(awk '$1 == "EMULATOR_USE_QT" { print $3; }' \
        $CONFIG_MAKE 2>/dev/null || true)
if [ "$EMULATOR_USE_QT" = "true" ]; then
    QT_PREBUILTS_DIR=$(awk '$1 == "QT_PREBUILTS_DIR" { print $3; }' \
            $CONFIG_MAKE 2>/dev/null || true)
    if [ ! -d "$QT_PREBUILTS_DIR" ]; then
        panic "Missing Qt prebuilts directory: $QT_PREBUILTS_DIR"
    fi
    echo "Copying Qt prebuilt libraries from $QT_PREBUILTS_DIR"
    for QT_ARCH in x86 x86_64; do
        QT_SRCDIR=$QT_PREBUILTS_DIR/$TARGET_OS-$QT_ARCH
        case $QT_ARCH in
            x86) QT_DSTDIR=$OUT_DIR/lib/qt;;
            x86_64) QT_DSTDIR=$OUT_DIR/lib64/qt;;
            *) panic "Invalid Qt host architecture: $QT_ARCH";;
        esac
        run mkdir -p "$QT_DSTDIR" || panic "Could not create Qt library sub-directory!"
        if [ ! -d "$QT_SRCDIR" ]; then
            continue
        fi
        case $TARGET_OS in
            windows) QT_DLL_FILTER="*.dll";;
            darwin) QT_DLL_FILTER="*.dylib";;
            *) QT_DLL_FILTER="*.so*";;
        esac
        QT_LIBS=$(cd "$QT_SRCDIR" && find . -name "$QT_DLL_FILTER" 2>/dev/null)
        #echo "QT_SRCDIR [$QT_SRCDIR] QT_LIBS [$QT_LIBS]"
        if [ -z "$QT_LIBS" ]; then
            panic "Cannot find Qt prebuilt libraries!?"
        fi
        for QT_LIB in $QT_LIBS; do
            QT_LIB=${QT_LIB#./}
            QT_DST_LIB=$QT_LIB
            if [ "$TARGET_OS" = "windows" ]; then
                # NOTE: On Windows, the prebuilt libraries are placed under
                # $PREBUILTS/qt/bin, not $PREBUILTS/qt/lib, ensure that they
                # are always copied to $OUT_DIR/lib[64]/qt/lib/.
                QT_DST_LIB=$(printf %s "$QT_DST_LIB" | sed -e 's|^bin/|lib/|g')
            fi
            copy_file "$QT_SRCDIR/$QT_LIB" "$QT_DSTDIR/$QT_DST_LIB"
        done
    done
fi

# Copy e2fsprogs binaries.
E2FSPROGS_DIR=$PREBUILTS_DIR/common/e2fsprogs
if [ ! -d "$E2FSPROGS_DIR" ]; then
    panic "Missing e2fsprogs prebuilts directory: $E2FSPROGS_DIR"
else
    echo "Copying e2fsprogs binaries."
    for E2FS_ARCH in x86 x86_64; do
        # NOTE: in windows only 32-bit binaries are available, so we'll copy the
        # 32-bit executables to the bin64/ directory to cover all our bases
        case $TARGET_OS in
            windows) E2FS_SRCDIR=$E2FSPROGS_DIR/$TARGET_OS-x86;;
            *) E2FS_SRCDIR=$E2FSPROGS_DIR/$TARGET_OS-$E2FS_ARCH;;
        esac

        case $E2FS_ARCH in
            x86) E2FS_DSTDIR=$OUT_DIR/bin;;
            x86_64) E2FS_DSTDIR=$OUT_DIR/bin64;;
            *) panic "Invalid e2fsprogs host architecture: $E2FS_ARCH"
        esac
        run mkdir -p "$E2FS_DSTDIR" || panic "Could not create sub-directory: $E2FS_DSTDIR"
        if [ ! -d "$E2FS_SRCDIR" ]; then
            continue
        fi
        run cp -a "$E2FS_SRCDIR"/sbin/* "$E2FS_DSTDIR" || "Could not copy e2fsprogs binaries!"
    done
fi

RUN_32BIT_TESTS=
RUN_64BIT_TESTS=true
RUN_EMUGEN_TESTS=true
RUN_GEN_ENTRIES_TESTS=true

TEST_SHELL=
EXE_SUFFIX=
if [ "$MINGW" ]; then
  TEST_SHELL=wine
  EXE_SUFFIX=.exe

  # Check for Wine on this machine.
  WINE_CMD=$(which $TEST_SHELL 2>/dev/null || true)
  if [ -z "$NO_TESTS" -a -z "$WINE_CMD" ]; then
    echo "WARNING: Wine is not installed on this machine!! Unit tests will be ignored!!"
    NO_TESTS=true
  fi

  RUN_32BIT_TESTS=true
fi

if [ "$HOST_OS" = "Linux" ]; then
    # Linux 32-bit binaries are deprecated but not removed yet!
    RUN_32BIT_TESTS=true
fi

# Return the minimum OS X version that a Darwin binary targets.
# $1: executable path
# Out: minimum version (e.g. '10.8') or empty string on error.
darwin_min_version () {
    otool -l "$1" 2>/dev/null | awk \
          'BEGIN { CMD="" } $1 == "cmd" { CMD=$2 } $1 == "version" && CMD == "LC_VERSION_MIN_MACOSX" { print $2 }'
}

OSX_DEPLOYMENT_TARGET=10.8

# List all executables to check later.
EXECUTABLES="emulator emulator64-arm emulator64-x86 emulator64-mips"
EXECUTABLES="$EXECUTABLES emulator64-ranchu-arm64 emulator64-ranchu-mips64"
EXECUTABLES="$EXECUTABLES emulator64-ranchu-x86 emulator64-ranchu-x86_64"
if [ "$HOST_OS" != "Darwin" ]; then
    EXECUTABLES="$EXECUTABLES emulator-arm emulator-x86 emulator-mips"
    EXECUTABLES="$EXECUTABLES emulator-ranchu-arm64 emulator-ranchu-mips64"
    EXECUTABLES="$EXECUTABLES emulator-ranchu-x86 emulator-ranchu-x86_64"
fi

if [ -z "$NO_TESTS" ]; then
    FAILURES=""

    echo "Checking for 'emulator' launcher program."
    EMULATOR=$OUT_DIR/emulator$EXE_SUFFIX
    if [ ! -f "$EMULATOR" ]; then
        echo "    - FAIL: $EMULATOR is missing!"
        FAILURES="$FAILURES emulator"
    fi

    echo "Checking that 'emulator' is a $EXPECTED_EMULATOR_BITNESS-bit program."
    EMULATOR_FILE_TYPE=$(get_file_type "$EMULATOR")
    if ! check_file_type_substring "$EMULATOR_FILE_TYPE" "$EXPECTED_EMULATOR_FILE_TYPE"; then
        echo "    - FAIL: $EMULATOR is not a 32-bit executable!"
        echo "        File type: $EMULATOR_FILE_TYPE"
        echo "        Expected : $EXPECTED_EMULATOR_FILE_TYPE"
        FAILURES="$FAILURES emulator-bitness-check"
    fi

    if [ "$HOST_OS" = "Darwin" ]; then
        echo "Checking that Darwin binaries target OSX $OSX_DEPLOYMENT_TARGET"
        for EXEC in $EXECUTABLES; do
            MIN_VERSION=$(darwin_min_version "$OUT_DIR/$EXEC")
            if [ "$MIN_VERSION" != "$OSX_DEPLOYMENT_TARGET" ]; then
                echo "   - FAIL: $EXEC targets [$MIN_VERSION], expected [$OSX_DEPLOYMENT_TARGET]"
                FAILURES="$FAILURES $EXEC-darwin-target"
            fi
        done
    fi

    if [ "$RUN_32BIT_TESTS" ]; then
        echo "Running 32-bit unit test suite."
        for UNIT_TEST in emulator_unittests emugl_common_host_unittests android_skin_unittests; do
        echo "   - $UNIT_TEST"
        run $TEST_SHELL $OUT_DIR/$UNIT_TEST$EXE_SUFFIX || FAILURES="$FAILURES $UNIT_TEST"
        done
    fi

    if [ "$RUN_64BIT_TESTS" ]; then
        echo "Running 64-bit unit test suite."
        for UNIT_TEST in emulator64_unittests emugl64_common_host_unittests android64_skin_unittests; do
            echo "   - $UNIT_TEST"
            run $TEST_SHELL $OUT_DIR/$UNIT_TEST$EXE_SUFFIX || FAILURES="$FAILURES $UNIT_TEST"
        done
    fi

    if [ "$RUN_EMUGEN_TESTS" ]; then
        EMUGEN_UNITTESTS=$OUT_DIR/emugen_unittests
        if [ ! -f "$EMUGEN_UNITTESTS" ]; then
            echo "FAIL: Missing binary: $EMUGEN_UNITTESTS"
            FAILURES="$FAILURES emugen_unittests-binary"
        else
            echo "Running emugen_unittests."
            run $EMUGEN_UNITTESTS ||
                FAILURES="$FAILURES emugen_unittests"
        fi
        echo "Running emugen regression test suite."
        # Note that the binary is always built for the 'build' machine type,
        # I.e. if --mingw is used, it's still a Linux executable.
        EMUGEN=$OUT_DIR/emugen
        if [ ! -f "$EMUGEN" ]; then
            echo "FAIL: Missing 'emugen' binary: $EMUGEN"
            FAILURES="$FAILURES emugen-binary"
        else
            # The first case is for a remote build with package-release.sh
            TEST_SCRIPT=$PROGDIR/../opengl/host/tools/emugen/tests/run-tests.sh
            if [ ! -f "$TEST_SCRIPT" ]; then
                # This is the usual location.
                TEST_SCRIPT=$PROGDIR/distrib/android-emugl/host/tools/emugen/tests/run-tests.sh
            fi
            if [ ! -f "$TEST_SCRIPT" ]; then
                echo " FAIL: Missing script: $TEST_SCRIPT"
                FAILURES="$FAILURES emugen-test-script"
            else
                run $TEST_SCRIPT --emugen=$EMUGEN ||
                    FAILURES="$FAILURES emugen-test-suite"
            fi
        fi
    fi

    # Check the gen-entries.py script.
    if [ "$RUN_GEN_ENTRIES_TESTS" ]; then
        echo "Running gen-entries.py test suite."
        run android/scripts/tests/gen-entries/run-tests.sh ||
            FAILURES="$FAILURES gen-entries_tests"
    fi

    # Check that the windows executables all have icons.
    # First need to locate the windres tool.
    if [ "$MINGW" ]; then
        echo "Checking windows executables icons."
        if [ ! -f "$CONFIG_MAKE" ]; then
            echo "FAIL: Could not find \$CONFIG_MAKE !?"
            FAILURES="$FAILURES out-dir-config-make"
        else
            WINDRES=$(awk '/^HOST_WINDRES:=/ { print $2; } $1 == "HOST_WINDRES" { print $3; }' $CONFIG_MAKE) ||
            if true; then
                echo "FAIL: Could not find host 'windres' program"
                FAILURES="$FAILURES host-windres"
            fi
            EXPECTED_ICONS=14
            for EXEC in $EXECUTABLES; do
                EXEC=${EXEC}.exe
                if [ ! -f "$OUT_DIR"/$EXEC ]; then
                    echo "FAIL: Missing windows executable: $EXEC"
                    FAILURES="$FAILURES windows-$EXEC"
                else
                    NUM_ICONS=$($WINDRES --input-format=coff --output-format=rc $OUT_DIR/$EXEC | grep RT_ICON | wc -l)
                    if [ "$NUM_ICONS" != "$EXPECTED_ICONS" ]; then
                        echo "FAIL: Invalid icon count in $EXEC ($NUM_ICONS, expected $EXPECTED_ICONS)"
                        FAILURES="$FAILURES windows-$EXEC-icons"
                    fi
                fi
            done
        fi
    fi
    if [ "$FAILURES" ]; then
        panic "Unit test failures: $FAILURES"
    fi
else
    echo "Ignoring unit tests suite."
fi

echo "Done. !!"
