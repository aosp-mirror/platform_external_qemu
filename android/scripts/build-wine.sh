#!/bin/sh

# This script is used to rebuild bi-arch Wine (32-bit *and* 64-bit at the
# same time) on an Ubuntu 14.04 machine. The build requires the creation
# of a chroot container, hence your sudo password will be needed to run
# this correctly.
#
# Various parameters can be overriden by defining an environment variable
# before running this script. For example, if you have a local clone of the
# Wine git repository, use:
#
#    WINE_GIT=/path/to/wine-source-git <this-script>
#

. $(dirname "$0")/utils/common.shi

shell_import utils/option_parser.shi
shell_import utils/temp_dir.shi

# Run a command inside a chroot
# $1: Chroot name (e.g. wine_ubuntu_i386)
# $2: Start directory inside chroot
# $3+: Command
run_schroot () {
    local NAME=$1
    local DIR=$2
    shift; shift;
    run schroot -c $NAME -d $DIR -u root -- "$@"
}

PROGRAM_DESCRIPTION=\
"This script is used to build a custom bi-arch Wine (32-bit and 64-bit)
installation on Ubuntu 14.04, using a chroot to perform the 32-bit build.

By default, the build will happen in a temporary directory that will be
deleted after the script exits (even in case of error), unless you use the
--build-dir=<path> option instead.

NOTE: Each Wine installation wants to use ~/.wine by default, but this can
      be overriden by defining the WINEPREFIX environment variable.

      Two different Wine versions will not work correctly with the same
      Wine prefix. We recommend uninstalling any existing older Wine
      package from your system, or always defining WINEPREFIX to avoid
      conflicts.

NOTE2: You will have to run \$INSTALL_DIR/winecfg at least once to setup
       your Wine prefix directory properly. It is ok to cancel any proposed
       download at first.
"

PROGRAM_PARAMETERS="<install-path>"

WINE_GIT=git://source.winehq.org/git/wine.git
option_register_var "--wine-git=<url>" WINE_GIT \
        "Select alternate Wine source git repository."

WINE_VERSION=wine-1.8.5
option_register_var "--wine-version=<tag>" WINE_VERSION \
        "Select alternate Wine source branch."

UBUNTU_RELEASE=trusty
option_register_var "--ubuntu-release=<name>" UBUNTU_RELEASE \
        "Alternative Ubuntu distribution name for 32-bit chroot build."

UBUNTU_ARCHIVE=http://archive.ubuntu.com/ubuntu/
option_register_var "--ubuntu-archive=<url>" UBUNTU_ARCHIVE \
        "Alternative Ubuntu archive URL."

BUILD_DIR=
option_register_var "--build-dir=<path>" BUILD_DIR \
        "Specify non-temporary build directory."

# List of pre-requisites packages for the host and the 32-bit schroot.
WINE_PREREQS="flex bison xorg-dev libjpeg-dev libpulse-dev libgl1-mesa-dev"

NUM_JOBS=$(grep -c ^processor /proc/cpuinfo)
option_register_var "--jobs=<count>" NUM_JOBS \
        "Number of parallel build tasks."

option_parse "$@"

if [ "$PARAMETER_COUNT" != "1" ]; then
    panic "This script takes a single argument. See --help."
fi

PREFIX=$PARAMETER_1
log "Installation path: $PREFIX"

GROUP=$(groups | cut -f1 -d" ")

if [ -z "$BUILD_DIR" ]; then
    var_create_sudo_temp_dir WORK_DIR "build-wine-temp"
    run sudo chown -R $USER:$GROUP $WORK_DIR
else
    WORK_DIR=$BUILD_DIR
fi
log "Build directory: $WORK_DIR"

mkdir -p $WORK_DIR || panic "Could not create work directory: $WORK_DIR"

# Installing pre-requisites.
dump "Installing Wine build pre-requisites on the host..."
run sudo apt-get install --yes git-core schroot debootstrap $WINE_PREREQS ||
        panic "Could not install Wine pre-requisites."

# Cloning or updating the source tree.
SOURCE_SUBDIR=wine-source
SOURCE_DIR=$WORK_DIR/$SOURCE_SUBDIR
if [ -d "$SOURCE_DIR" ]; then
    dump "Resetting source tree to version $WINE_VERSION."
    (cd $SOURCE_DIR && git fetch && git clean -xfd &&
     git reset --hard $WINE_VERSION) ||
            panic "Unknown version $WINE_VERSION."
else
    dump "Cloning Wine source repository..."
    mkdir -p $SOURCE_DIR
    run git clone $WINE_GIT $SOURCE_DIR ||
            panic "git clone failed!"
    git -C $SOURCE_DIR checkout -q $WINE_VERSION ||
            panic "Unknown version $WINE_VERSION."
fi

dump "Create 32-bit schroot."
CHROOT_NAME=wine_ubuntu_i386
cat > /tmp/$CHROOT_NAME <<EOF
[$CHROOT_NAME]
description=Ubuntu Release 32-bit
personality=linux32
directory=$WORK_DIR/$CHROOT_NAME
root-users=$USER
type=directory
users=$USER
EOF

run sudo cp /tmp/$CHROOT_NAME /etc/schroot/chroot.d/$CHROOT_NAME ||
        panic "Could not create schroot $CHROOT_NAME"

rm -f /tmp/$CHROOT_NAME

CHROOT_TOP_DIR=$WORK_DIR/$CHROOT_NAME

run sudo mkdir -p "$CHROOT_TOP_DIR" || panic "Could not create schroot"

CHROOT_BUILD_DIR=/opt/wine-build
CHROOT_WORK_DIR=$CHROOT_TOP_DIR/$CHROOT_BUILD_DIR
sudo mkdir -p "$CHROOT_WORK_DIR" &&

run sudo debootstrap --variant=buildd --arch=i386 $UBUNTU_RELEASE \
     $WORK_DIR/$CHROOT_NAME $UBUNTU_ARCHIVE &&
run_schroot $CHROOT_NAME / apt-get update &&
run_schroot $CHROOT_NAME / /bin/bash -c "DEBIAN_FRONTEND=noninteractive apt-get install --yes \
        -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" \
        ubuntu-minimal software-properties-common \
        $WINE_PREREQS" ||
        panic "Could not setup schroot"

# Creating the 64-bit build directory.
dump "Perform 64-bit Wine build (binaries and libraries only)."
run mkdir -p $WORK_DIR/wine64-build || panic "Could not create 64-bit build dir."
run rm -rf $WORK_DIR/wine64-build/* &&
(run cd $WORK_DIR/wine64-build &&
 run $SOURCE_DIR/configure --enable-win64 --prefix=$PREFIX &&
 run make -j$NUM_JOBS) || panic "Could not perform 64-bit Wine build!"

run sudo cp -R $SOURCE_DIR $CHROOT_WORK_DIR/$SOURCE_SUBDIR &&
run sudo cp -R $WORK_DIR/wine64-build $CHROOT_WORK_DIR/wine64-build ||
        panic "Could not initialize schroot."

dump "Build 32-bit Wine tools in schroot."
run_schroot $CHROOT_NAME $CHROOT_BUILD_DIR mkdir wine32-tools-build &&
run_schroot $CHROOT_NAME $CHROOT_BUILD_DIR/wine32-tools-build \
        ../$SOURCE_SUBDIR/configure \
        --prefix=$PREFIX &&
run_schroot $CHROOT_NAME $CHROOT_BUILD_DIR/wine32-tools-build \
        make -j$NUM_JOBS || panic "Could not build 32-bit Wine tools."

dump "Build 32-bit Wine in schroot."
run_schroot $CHROOT_NAME $CHROOT_BUILD_DIR mkdir wine32-build &&
run_schroot $CHROOT_NAME $CHROOT_BUILD_DIR/wine32-build \
        ../$SOURCE_SUBDIR/configure \
        --prefix=$PREFIX --with-wine64=$CHROOT_BUILD_DIR/wine64-build \
        --with-wine-tools=$CHROOT_BUILD_DIR/wine32-tools-build &&
run_schroot $CHROOT_NAME $CHROOT_BUILD_DIR/wine32-build make -j$NUM_JOBS ||
        panic "Could not build 32-bit Wine."

dump "Installing 32-bit Wine binaries and libraries to $PREFIX."
run_schroot $CHROOT_NAME $CHROOT_BUILD_DIR/wine32-build make install &&
run sudo cp -rdP $WORK_DIR/$CHROOT_NAME/$PREFIX $PREFIX &&
run sudo chmod -R a+r $PREFIX &&
run sudo chown -R $USER:$GROUP $PREFIX ||
        panic "Could not install 32-bit Wine."

dump "Installing 64-bit Wine tools, binaries and libraries to $PREFIX."
(run cd $WORK_DIR/wine64-build && run make install) ||
        panic "Could not install 64-bit Wine binaries."

dump "Copying script to install dir"

DATE=$(date +%Y-%m-%d)
cat > $PREFIX/README <<EOF
This directory contains a build of Wine $WINE_VERSION for the $UBUNTU_RELEASE
Ubuntu release. It was built with the $(basename $0) script on $DATE.
To use it for the first time, run:

  export PATH=$PREFIX/bin:\$PATH
  winecfg

And select "Cancel" to each prompt that asks you if you want to
download things. This will setup your ~/.wine directory. You can
also save its content to another location, and define WINEPREFIX
in your environment to refer to it.

If you want to use it for Android emulation development, please run
the external/qemu/android/tests/prepare-wine-registry.sh script before
trying to run the Wine unit-tests through android/rebuild.sh.

EOF

if [ "$BUILD_DIR" ]; then
    echo "Done. Don't forget to remove your build directory: $BUILD_DIR"
else
    dump "Removing 32-bit chroot."
    run sudo rm -f /etc/schroot/chroot.d/$CHROOT_NAME

    echo "Done."
fi

# NOTE: The temporary build directory will be removed when this script exits!
