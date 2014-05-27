#!/bin/sh
#
# A small script used to rebuild the Android goldfish kernel image
# See docs/KERNEL.TXT for usage instructions.
#
MACHINE=goldfish
VARIANT=goldfish
OUTPUT=/tmp/kernel-qemu
CROSSPREFIX=arm-linux-androideabi-
CONFIG=goldfish
GCC_VERSION=4.7

VALID_ARCHS="arm x86 x86_64 mips"

# Determine the host architecture, and which default prebuilt tag we need.
# For the toolchain auto-detection.
#
HOST_OS=`uname -s`
case "$HOST_OS" in
    Darwin)
        HOST_OS=darwin
        HOST_TAG=darwin-x86
        BUILD_NUM_CPUS=$(sysctl -n hw.ncpu)
        ;;
    Linux)
        # note that building  32-bit binaries on x86_64 is handled later
        HOST_OS=linux
        HOST_TAG=linux-x86
        BUILD_NUM_CPUS=$(grep -c processor /proc/cpuinfo)
        ;;
    *)
        echo "ERROR: Unsupported OS: $HOST_OS"
        exit 1
esac

# Default number of parallel jobs during the build: cores * 2
JOBS=$(( $BUILD_NUM_CPUS * 2 ))

ARCH=arm

OPTION_HELP=no
OPTION_ARMV7=yes
OPTION_OUT=
OPTION_CROSS=
OPTION_ARCH=
OPTION_CONFIG=
OPTION_SAVEDEFCONFIG=no
OPTION_JOBS=
OPTION_VERBOSE=
OPTION_GCC_VERSION=
CCACHE=

case "$USE_CCACHE" in
    "")
        CCACHE=
        ;;
    *)
        # use ccache bundled in AOSP source tree
        CCACHE=${ANDROID_BUILD_TOP:-$(dirname $0)/../../..}/prebuilts/misc/$HOST_TAG/ccache/ccache
        [ -x $CCACHE ] || CCACHE=
        ;;
esac

for opt do
    optarg=$(expr "x$opt" : 'x[^=]*=\(.*\)')
    case $opt in
    --help|-h|-\?) OPTION_HELP=yes
        ;;
    --arch=*)
        OPTION_ARCH=$optarg
        ;;
    --armv5)
        OPTION_ARMV7=no
        ;;
    --armv7)
        OPTION_ARMV7=yes
        ;;
    --ccache=*)
        CCACHE=$optarg
        ;;
    --config=*)
        OPTION_CONFIG=$optarg
        ;;
    --cross=*)
        OPTION_CROSS=$optarg
        ;;
    --gcc-version=*)
        OPTION_GCC_VERSION=$optarg
        ;;
    -j*|--jobs=*)
        OPTION_JOBS=$optarg
        ;;
    --out=*)
        OPTION_OUT=$optarg
        ;;
    --savedefconfig)
        OPTION_SAVEDEFCONFIG=yes
        ;;
    --verbose)
        OPTION_VERBOSE=true
        ;;
    *)
        echo "unknown option '$opt', use --help"
        exit 1
    esac
done

if [ $OPTION_HELP = "yes" ] ; then
    echo "Rebuild the prebuilt kernel binary for Android's emulator."
    echo ""
    echo "options (defaults are within brackets):"
    echo ""
    echo "  --help                   print this message"
    echo "  --arch=<arch>            change target architecture [$ARCH]"
    echo "  --armv5                  build ARMv5 binaries"
    echo "  --armv7                  build ARMv7 binaries (default. see note below)"
    echo "  --out=<directory>        output directory [$OUTPUT]"
    echo "  --cross=<prefix>         cross-toolchain prefix [$CROSSPREFIX]"
    echo "  --config=<name>          kernel config name [$CONFIG]"
    echo "  --savedefconfig          run savedefconfig"
    echo "  --ccache=<path>          use compiler cache [${CCACHE:-not set}]"
    echo "  --gcc-version=<version>  use specific GCC version [$GCC_VERSION]"
    echo "  --verbose                show build commands"
    echo "  -j<number>               launch <number> parallel build jobs [$JOBS]"
    echo ""
    echo "NOTE: --armv7 is equivalent to --config=goldfish_armv7. It is"
    echo "      ignored if --config=<name> is used."
    echo ""
    exit 0
fi

if [ ! -f include/linux/vermagic.h ]; then
    echo "ERROR: You must be in the top-level kernel source directory to run this script."
    exit 1
fi

# Extract kernel version, we'll need to put this in the final binaries names
# to ensure the emulator can trivially know it without probing the binary with
# 'file' or other unreliable heuristics.
KERNEL_MAJOR=$(awk '$1 == "VERSION" { print $3; }' Makefile)
KERNEL_MINOR=$(awk '$1 == "PATCHLEVEL" { print $3; }' Makefile)
KERNEL_PATCH=$(awk '$1 == "SUBLEVEL" { print $3; }' Makefile)
KERNEL_VERSION="$KERNEL_MAJOR.$KERNEL_MINOR.$KERNEL_PATCH"
echo "Found kernel version: $KERNEL_VERSION"

if [ -n "$OPTION_ARCH" ]; then
    ARCH=$OPTION_ARCH
fi

if [ -n "$OPTION_GCC_VERSION" ]; then
    GCC_VERSION=$OPTION_GCC_VERSION
else
    if [ "$ARCH" = "x86" ]; then
        # Work-around a nasty bug.
        # Hence 132637 is 2.6.29.
        if [ "$KERNEL_VERSION" = "2.6.29" ]; then
            GCC_VERSION=4.6
            echo "WARNING: android-goldfish-$KERNEL_VERSION doesn't build --arch=$ARCH with GCC 4.7"
        fi
    fi
    if [ "$ARCH" = "arm64" ]; then
        # There is no GCC 4.7 toolchain to build AARCH64 binaries.
        GCC_VERSION=4.8
    fi
    echo "Autoconfig: --gcc-version=$GCC_VERSION"
fi

if [ -n "$OPTION_CONFIG" ]; then
    CONFIG=$OPTION_CONFIG
else
    case $ARCH in
        arm)
            CONFIG=goldfish_armv7
            if  [ "$OPTION_ARMV5" = "yes" ]; then
                CONFIG=goldfish
            fi
            ;;
        x86)
            # Warning: this is ambiguous, should be 'goldfish' before 3.10,
            # and 'i386_emu" after it.
            if [ -f "arch/x86/configs/i386_emu_defconfig" ]; then
                CONFIG=i386_emu
            else
                CONFIG=goldfish
            fi
            ;;
        x86_64)
            CONFIG=x86_64_emu
            ;;
        mips)
            CONFIG=goldfish
            ;;
        arm64)
            # TODO(digit): Provide better config.
            CONFIG=defconfig
            ;;
        *)
            echo "ERROR: Invalid arch '$ARCH', try one of $VALID_ARCHS"
            exit 1
    esac
    echo "Auto-config: --config=$CONFIG"
fi

# Check output directory.
if [ -n "$OPTION_OUT" ] ; then
    if [ ! -d "$OPTION_OUT" ] ; then
        echo "Output directory '$OPTION_OUT' does not exist ! Aborting."
        exit 1
    fi
    OUTPUT=$OPTION_OUT
else
    mkdir -p $OUTPUT
fi

if [ -n "$OPTION_CROSS" ] ; then
    CROSSPREFIX="$OPTION_CROSS"
else
    case $ARCH in
        arm)
            CROSSPREFIX=arm-linux-androideabi-
            ;;
        x86)
            CROSSPREFIX=i686-linux-android-
            ;;
        x86_64)
            CROSSPREFIX=x86_64-linux-android-
            ;;
        mips)
            CROSSPREFIX=mipsel-linux-android-
            ;;
        arm64)
            CROSSPREFIX=aarch64-linux-android-
            ;;
        *)
            echo "ERROR: Unsupported architecture!"
            exit 1
            ;;
    esac
    CROSSTOOLCHAIN=${CROSSPREFIX}$GCC_VERSION
    echo "Auto-config: --cross=$CROSSPREFIX"
fi

ZIMAGE=zImage

case $ARCH in
    x86|x86_64)
        ZIMAGE=bzImage
        ;;
    arm64)
        ZIMAGE=Image.gz
        ;;
    mips)
        ZIMAGE=
        ;;
esac

# If the cross-compiler is not in the path, try to find it automatically
CROSS_COMPILER="${CROSSPREFIX}gcc"
CROSS_COMPILER_VERSION=$($CROSS_COMPILER --version 2>/dev/null)
if [ $? != 0 ] ; then
    BUILD_TOP=$ANDROID_BUILD_TOP
    if [ -z "$BUILD_TOP" ]; then
        # Assume this script is under external/qemu/distrib/ in the
        # Android source tree.
        BUILD_TOP=$(dirname $0)/../../..
        if [ ! -d "$BUILD_TOP/prebuilts" ]; then
            BUILD_TOP=
        else
            BUILD_TOP=$(cd $BUILD_TOP && pwd)
        fi
    fi
    case $ARCH in
        x86_64)
            # x86_46 binaries are under prebuilts/gcc/<host>/x86 !!
            PREBUILT_ARCH=x86
            ;;
        arm64)
            PREBUILT_ARCH=aarch64
            ;;
        *)
            PREBUILT_ARCH=$ARCH
            ;;
    esac
    CROSSPREFIX=$BUILD_TOP/prebuilts/gcc/$HOST_TAG/$PREBUILT_ARCH/$CROSSTOOLCHAIN/bin/$CROSSPREFIX
    echo "Checking for ${CROSSPREFIX}gcc"
    if [ "$BUILD_TOP" -a -f ${CROSSPREFIX}gcc ]; then
        echo "Auto-config: --cross=$CROSSPREFIX"
    else
        echo "It looks like $CROSS_COMPILER is not in your path ! Aborting."
        exit 1
    fi
fi

if [ "$CCACHE" ] ; then
    echo "Using ccache program: $CCACHE"
    CROSSPREFIX="$CCACHE $CROSSPREFIX"
fi

export CROSS_COMPILE="$CROSSPREFIX" ARCH SUBARCH=$ARCH

if [ "$OPTION_JOBS" ]; then
    JOBS=$OPTION_JOBS
else
    echo "Auto-config: -j$JOBS"
fi


# Special magic redirection with our magic toolbox script
# This is needed to add extra compiler flags to compiler.
# See kernel-toolchain/android-kernel-toolchain-* for details
#
export REAL_CROSS_COMPILE="$CROSS_COMPILE"
CROSS_COMPILE=$(dirname "$0")/kernel-toolchain/android-kernel-toolchain-

MAKE_FLAGS=
if [ "$OPTION_VERBOSE" ]; then
  MAKE_FLAGS="$MAKE_FLAGS V=1"
fi

case $CONFIG in
    defconfig)
        MAKE_DEFCONFIG=$CONFIG
        ;;
    *)
        MAKE_DEFCONFIG=${CONFIG}_defconfig
        ;;
esac

# Do the build
#
rm -f include/asm &&
make $MAKE_DEFCONFIG &&    # configure the kernel
make -j$JOBS $MAKE_FLAGS       # build it

if [ $? != 0 ] ; then
    echo "Could not build the kernel. Aborting !"
    exit 1
fi

if [ "$OPTION_SAVEDEFCONFIG" = "yes" ]; then
    make savedefconfig
    mv -f defconfig arch/$ARCH/configs/${CONFIG}_defconfig
fi

# Note: The exact names of the output files are important for the Android build,
#       do not change the definitions lightly.
KERNEL_PREFIX=kernel-$KERNEL_VERSION

case $CONFIG in
    vbox*)
        KERNEL_SUFFIX=vbox
        ;;
    goldfish)
        if [ "$ARCH" = "arm" ]; then
            KERNEL_SUFFIX=qemu-armv5
        else
            KERNEL_SUFFIX=qemu-$ARCH
        fi
        ;;
    goldfish_armv7)
        KERNEL_SUFFIX=qemu-armv7
        ;;
    *)
        KERNEL_SUFFIX=qemu-$ARCH
esac
OUTPUT_KERNEL=$KERNEL_PREFIX-$KERNEL_SUFFIX
OUTPUT_VMLINUX=vmlinux-${OUTPUT_KERNEL##kernel-}

cp -f vmlinux $OUTPUT/$OUTPUT_VMLINUX
if [ ! -z $ZIMAGE ]; then
    cp -f arch/$ARCH/boot/$ZIMAGE $OUTPUT/$OUTPUT_KERNEL
else
    cp -f vmlinux $OUTPUT/$OUTPUT_KERNEL
fi
echo "Kernel $CONFIG prebuilt images ($OUTPUT_KERNEL and $OUTPUT_VMLINUX) copied to $OUTPUT successfully !"

exit 0
