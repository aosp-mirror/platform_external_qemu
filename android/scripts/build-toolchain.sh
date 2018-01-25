#!/bin/sh

# Copyright 2018 The Android Open Source Project
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
shell_import utils/temp_dir.shi
shell_import utils/task.shi

_ack_ () {
  FOR_REALZ=yes
}

PROGRAM_PARAMETERS="<install-dir>"

PROGRAM_DESCRIPTION=\
"Generates a toolchain used to compile the emulator.

 This will generate a toolchain by using crosstool to construct
 the compiler and a basic sysroot.

 Next we will merge in additional packages from ubuntu to make sure
 we can link and refer to libraries we rely on in the emulator.
"


OPT_UBUNTU_MIRROR=http://mirrors.us.kernel.org
option_register_var "--mirror=<url>" OPT_UBUNTU_MIRROR "The base URL of the Ubuntu mirror we're going to use."

OPT_UBUNTU_RELEASE=xenial
option_register_var "--release=<release>" OPT_UBUNTU_RELEASE "Ubuntu release name we want packages from. Can be a name or a number (i.e.  xenial or 16.04)"

OPT_CROSTOOL_NG_URL=http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-1.23.0.tar.xz
option_register_var "--crosstool=<url>" OPT_CROSTOOL_NG_URL "URL to Crosstool-NG toolchain generator"

OPT_CROSSTOOL_CONFIG=x86_64-ubuntu16.04-linux-gnu
option_register_var "--cnf=<config>" OPT_CROSSTOOL_CONFIG "Config file from crosstool to use as toolchain."

OPT_NUM_JOBS=
option_register_var "-j<count>" OPT_NUM_JOBS "Run <count> parallel build jobs [$(get_build_num_cores)]"
option_register_var "--jobs=<count>" OPT_NUM_JOBS "Same as -j<count>."

# var_create_temp_dir OPT_WORK_DIR toolchain-temp
OPT_WORK_DIR=/tmp/gcc-xxx
option_register_var "--work-dir=<dir>" OPT_WORK_DIR "Temporary work directory"

# Name of the final toolchain binary tarball that this script will create
TOOLCHAIN_ARCHIVE=
option_register_var "--archive=<zip/tar>" TOOLCHAIN_ARCHIVE "Final archive with the toolchain"

FOR_REALZ=no
option_register --ack _ack_ "I understand that I will have to re-compile all dependencies, fix the build, and other toolchain voodoo"

option_parse "$@"

if [ "$OPT_WORK_DIR" ==  "/tmp/gcc-xxx" ]; then
  var_create_temp_dir OPT_WORK_DIR toolchain-temp
fi

if [ "$TOOLCHAIN_ARCHIVE" == "" ]; then
  TOOLCHAIN_ARCHIVE=/tmp/$OPT_CROSSTOOL_CONFIG.zip
fi

RUN_CONFIG=$(echo $OPT_CROSSTOOL_CONFIG|  sed 's/[\.-]/_/g')

if [ "$FOR_REALZ" != "yes" ]; then
  panic "You have to acknowledge that you will do all the things to make the build work."
else
  echo "Ok... Trying to create a toolchain. This usually takes about 30 minutes or so"
fi

if [ "$OPT_NUM_JOBS" ]; then
    NUM_JOBS=$OPT_NUM_JOBS
    log "Parallel jobs count: $NUM_JOBS"
else
    NUM_JOBS=$(get_build_num_cores)
    log "Auto-config: --jobs=$NUM_JOBS"
fi


## VARIABLES WE USE

BUILD_DIR=$OPT_WORK_DIR/build
BUILD_DIR_BIN=$OPT_WORK_DIR/tmpbin
TMPLOG=$BUILD_DIR/build.log
TIMESTAMPS_DIR=$BUILD_DIR/timestamps
DOWNLOAD_DIR=$OPT_WORK_DIR/download
ORG_SYSROOT_DIR=$OPT_WORK_DIR/unpacked_sysroot
FINAL_TOOLCHAIN="$OPT_WORK_DIR/final"
INTER_TOOLCHAIN=$OPT_WORK_DIR/inter
CROSSTOOL="${OPT_CROSTOOL_NG_URL##*/}"
CROSSTOOL_DIR=$(echo $CROSSTOOL | sed 's/.tar.*//g')


UBUNTU_ARCHS="amd64"
UBUNTU_REPOS="main multiverse universe"

mkdir -p $BUILD_DIR
rm -rf $TMPLOG && touch $TMPLOG
mkdir -p $TIMESTAMPS_DIR
mkdir -p $BUILD_DIR_BIN


UBUNTU_PACKAGES=
add_ubuntu_package ()
{
    UBUNTU_PACKAGES="$UBUNTU_PACKAGES $@"
}

# The package files containing kernel headers for Hardy and the C
# library headers and binaries
add_ubuntu_package \
    linux-libc-dev \
    libc6 \
    libc6-dev \
    libcap2 \
    libcap-dev \
    libattr1 \
    libattr1-dev \
    libacl1 \
    libacl1-dev \

# The X11 headers and binaries (for the emulator)
add_ubuntu_package \
    libx11-6 \
    libx11-dev \
    libxau6 \
    libxcb1 \
    libxcb1-dev \
    libxcb-dri3-0 \
    libxdmcp6 \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    x11proto-core-dev \
    x11proto-fixes-dev \
    x11proto-xext-dev \
    x11proto-input-dev \
    x11proto-kb-dev

# The OpenGL-related headers and libraries (for GLES emulation)
add_ubuntu_package \
    libdrm2 \
    libexpat1 \
    libgl1-mesa-dev \
    libgl1-mesa-glx \
    libglapi-mesa \
    libx11-xcb1 \
    libxcb-dri2-0 \
    libxcb-glx0 \
    libxcb-present0 \
    libxcb-sync1 \
    libxdamage1 \
    libxext6 \
    libxfixes3 \
    libxshmfence1 \
    libxxf86vm1 \
    mesa-common-dev \


# Audio libraries (required by the emulator)
add_ubuntu_package \
    libasound2 \
    libasound2-dev \
    libesd0-dev \
    libaudiofile-dev \
    libpulse0 \
    libjson-c2 \
    libdbus-1-3 \
    libpulse-dev \
    libsystemd0 \
    libwrap0 \
    libsndfile1 \
    libasyncns0 \
    libselinux1 \
    libflac8 \
    libogg0 \
    libvorbis0a \
    libvorbisenc2 \


# ZLib and others.
add_ubuntu_package \
    zlib1g \
    zlib1g-dev \
    libncurses5 \
    libncurses5-dev \
    libtinfo5 \
    liblzma5 \
    libgcrypt20 \
    libtinfo-dev \
    libpcre3 \
    libgpg-error0 \

# Fancy colors
RED=`tput setaf 1`
RESET=`tput sgr0`

# Download a file.
# $1: Source URL
# $2: Destination directory.
# $3: [Optional] expected SHA-1 sum of downloaded file.
download_package () {
    # Assume the packages are already downloaded under $ARCHIVE_DIR
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


# Load the Ubuntu packages file. This is a long text file that will list
# each package for a given release.
#
# $1: Ubuntu mirror base URL (e.g. http://mirrors.us.kernel.org/)
# $2: Release name
#
get_ubuntu_packages_list ()
{
    local RELEASE=$2
    local BASE="`no_trailing_slash \"$1\"`"
    local SRCFILE DSTFILE
    for UA in $UBUNTU_ARCHS; do
      if [ -e $DOWNLOAD_DIR/Packages-$UA¶ ]; then
        rm $DOWNLOAD_DIR/Packages-$UA
      fi
      for REPO in $UBUNTU_REPOS; do
        SRCFILE="$BASE/ubuntu/dists/$RELEASE/$REPO/binary-$UA/Packages.gz"
        DSTFILE="$DOWNLOAD_DIR/Packages-$REPO-$UA.gz"
        log "Trying to load $SRCFILE"
        download_file "$SRCFILE" "$DSTFILE" || panic "Could not download $SRCFILE"
        (cd $DOWNLOAD_DIR && gunzip -cf Packages-$REPO-$UA.gz >> Packages-$UA) || panic "Could not uncompress $DSTFILE to Packages-$UA"
      done
    done

    # Write a small awk script used to extract filenames for a given package
    cat > $DOWNLOAD_DIR/extract-filename.awk <<EOF
BEGIN {
    # escape special characters in package name
    gsub("\\\\.","\\\\.",PKG)
    gsub("\\\\+","\\\\+",PKG)
    FILE = ""
    PACKAGE = ""
}

\$1 == "Package:" {
    if (\$2 == PKG) {
        PACKAGE = \$2
    } else {
        PACKAGE = ""
    }
}

\$1 == "Filename:" && PACKAGE == PKG {
    FILE = \$2
}

END {
    print FILE
}
EOF
}

# Convert an unversioned package name into a .deb package URL
#
# $1: Package name without version information (e.g. libc6-dev)
# $2: Ubuntu mirror base URL
# $3: Ubuntu arch ("i386" or "amd64")
#
get_ubuntu_package_deb_url ()
{
    # The following is an awk command to parse the Packages file and extract
    # the filename of a given package.
    local BASE="`no_trailing_slash \"$1\"`"
    local FILE=`awk -f "$DOWNLOAD_DIR/extract-filename.awk" -v PKG=$1 $DOWNLOAD_DIR/Packages-$3`
    if [ -z "$FILE" ]; then
        log "Could not find filename for package $1"
        exit 1
    fi
    echo "$2/ubuntu/$FILE"
}


task_define download_ubuntu_packages_list "Download Ubuntu packages list"
cmd_download_ubuntu_packages_list ()
{
    mkdir -p $DOWNLOAD_DIR
    get_ubuntu_packages_list "$OPT_UBUNTU_MIRROR" "$OPT_UBUNTU_RELEASE" ||  panic "Unable to download packages list, try --ubuntu-mirror=<url> to use another archive mirror"
}

task_define download_packages "Download Ubuntu packages"
task_depends download_packages download_ubuntu_packages_list
cmd_download_packages ()
{
    local PACKAGE PKGURL
    rm -f $DOWNLOAD_DIR/SOURCES && touch $DOWNLOAD_DIR/SOURCES
    for PACKAGE in $UBUNTU_PACKAGES; do
        log "Downloading $PACKAGE"
        for UA in $UBUNTU_ARCHS; do
            PKGURL=`get_ubuntu_package_deb_url $PACKAGE $OPT_UBUNTU_MIRROR $UA`
            download_file_to $PKGURL $DOWNLOAD_DIR || panic "Could not download $PKGURL"
        done
    done
    sha1sum $DOWNLOAD_DIR/*.deb | while read LINE; do
        PACKAGE=$(basename $(echo $LINE | awk '{ print $2;}'))
        SHA1=$(echo $LINE | awk '{ print $1; }')
        printf "%-64s %s\n" $PACKAGE $SHA1 >> $DOWNLOAD_DIR/SOURCES
    done
}

task_define build_sysroot "Build sysroot"
task_depends build_sysroot download_packages

cmd_build_sysroot ()
{
    local PACKAGE PKGURL SRC_PKG
    rm -rf $ORG_SYSROOT_DIR
    mkdir -p $SRC_PKG $ORG_SYSROOT_DIR
    for PACKAGE in $UBUNTU_PACKAGES; do
      for UA in $UBUNTU_ARCHS; do
        PKGURL=`get_ubuntu_package_deb_url $PACKAGE $OPT_UBUNTU_MIRROR $UA`
        SRC_PKG=$DOWNLOAD_DIR/`basename $PKGURL`
        log "Extracting $SRC_PKG to $ORG_SYSROOT_DIR"
        dpkg -x $SRC_PKG $ORG_SYSROOT_DIR/$UA
      done
    done
}


task_define download_crosstool "Download crosstool"

cmd_download_crosstool ()
{
  download_file_to $OPT_CROSTOOL_NG_URL $DOWNLOAD_DIR
}


task_define configure_crosstool "Configure crosstool"
task_depends configure_crosstool download_crosstool
cmd_configure_crosstool ()
{
  unpack_archive "$DOWNLOAD_DIR/$CROSSTOOL" "$BUILD_DIR"
  cd $BUILD_DIR/$CROSSTOOL_DIR
  ./configure --enable-local >> $TMPLOG
}


task_define compile_crosstool "Compile crosstool"
task_depends compile_crosstool configure_crosstool
cmd_compile_crosstool ()
{
  cd $BUILD_DIR/$CROSSTOOL_DIR
  make
}


task_define build_x86_64_ubuntu16_04_linux_gnu_toolchain "Build the ubuntu toolchain"
task_depends build_x86_64_ubuntu16_04_linux_gnu_toolchain compile_crosstool
cmd_build_x86_64_ubuntu16_04_linux_gnu_toolchain ()
{
  cd $BUILD_DIR/$CROSSTOOL_DIR

  CONFIG=samples/$OPT_CROSSTOOL_CONFIG/crosstool.config
  # First we need to fix up our config.
  echo "CT_WANTS_STATIC_LINK=y" >> $CONFIG
  echo "CT_WANTS_STATIC_LINK_CXX=y" >> $CONFIG
  echo "CT_STATIC_TOOLCHAIN=y" >> $CONFIG

  CT_PREFIX=$INTER_TOOLCHAIN ./ct-ng $OPT_CROSSTOOL_CONFIG || panic "Unable to prepare toolcahin"
  CT_PREFIX=$INTER_TOOLCHAIN ./ct-ng build.$NUM_JOBS || panic "Unable to build toolchain"
}

task_define fixup_sysroot "Fixup sysroot packages"
task_depends fixup_sysroot build_sysroot
cmd_fixup_sysroot()
{
  # We need to fixup pkgconfig
  find $ORG_SYSROOT_DIR -name '*.pc' -exec sed -i "s/\/x86_64-linux-gnu//g" {} \; || panic "Could'nt fix pkgconfig"

  local SYSROOT=$ORG_SYSROOT_DIR/amd64

  # Next we need to move some libs around
  for DIR in usr/lib ; do
    local LIB=$SYSROOT/$DIR/x86_64-linux-gnu
    # Let's fixup libs that are in the wrong place
    for PKGDIR in pulseaudio ; do
      local SUBPACKAGE=$LIB/$PKGDIR
      if [ -d "$SUBPACKAGE" ]; then
        mv $SUBPACKAGE/* `dirname $SUBPACKAGE` && rmdir $SUBPACKAGE || panic "Cannot move files in $SUBPACKAGE"
      else
        log  "$SUBPACKAGE does not exist"
      fi
    done
  done


  for DIR in usr/lib lib ; do
    local LIB=$SYSROOT/$DIR/x86_64-linux-gnu

    if [ -d "$LIB" ]; then
      mv $LIB/* `dirname $LIB` && rmdir $LIB || panic "Cannot move files in $LIB"
    else
      log "$LIB does not exist"
    fi
  done

  for LIB in lib ; do
    # We need to fix the symlink like librt.so -> /lib*/librt.so.1
    # in $(sysroot_dir)/usr/$LIB, they should point to librt.so.1 instead now.
    SYMLINKS=$(find_symlinks_in $SYSROOT/usr/$LIB)
    cd $SYSROOT/usr/$LIB
    for SL in $SYMLINKS; do
      # convert /$LIB/libfoo.so.<n> into 'libfoo.so.<n>' for the target
      local DST=$(readlink $SL 2>/dev/null)
      local DST2=`basename $DST`
      if [ "$DST2" != "$DST" ]; then
        log "Fixing symlink $SL --> $DST"
        if [ "${DST:0:1}" = "/" ]; then
          # absolute path.
          DST=$(echo $DST |sed 's/\/x86_64-linux-gnu//')
          RELTO=$(realpath --relative-to="$SYSROOT/usr/$LIB/" "$SYSROOT/$DST")
          log2 "Setting relative link as $SYSROOT/usr/$LIB/$SL --> $SYSROOT/$DST --> $RELTO"
          rm $SL && ln -s $RELTO $SL
        else
          # relative path
          log "Relative link as $SL --> $DST"
          rm $SL && ln -s $DST $SL
        fi

      fi
    done
  done

  # Ok, so some things might end up under x86_64-linux-gnu, which is bad news.
  if [ -d "$SYSROOT/usr/include/x86_64-linux-gnu" ]; then
    cp -r $SYSROOT/usr/include/x86_64-linux-gnu/* $SYSROOT/usr/include/
    rm -rf $SYSROOT/usr/include/x86_64-linux-gnu/
  else
    log "$SYSROOT/usr/include/x86_64-linux-gnu does not exist"
  fi

}

task_define merge_in_x86_64_ubuntu16_04_linux_gnu_sysroot "Merge in additional sysroot packages"
task_depends merge_in_x86_64_ubuntu16_04_linux_gnu_sysroot fixup_sysroot
cmd_merge_in_x86_64_ubuntu16_04_linux_gnu_sysroot()
{
  local SYSROOT=$ORG_SYSROOT_DIR/amd64/
  if [ -d "$FINAL_TOOLCHAIN" ]; then
    chmod -R +w $FINAL_TOOLCHAIN
    rm -rf $FINAL_TOOLCHAIN || panic "Unable to clean out old toolchain"
  fi

  mkdir -p $FINAL_TOOLCHAIN

  log "Populating the toolchain"
  cp -r $INTER_TOOLCHAIN/$OPT_CROSSTOOL_CONFIG/* $FINAL_TOOLCHAIN

  # Make the sysroot readable
  chmod -R +w $FINAL_TOOLCHAIN
  log "Syncing the remainder"
  rsync -raz --ignore-existing $SYSROOT $FINAL_TOOLCHAIN/$OPT_CROSSTOOL_CONFIG/sysroot
}

task_define merge_in_x86_64_w64_mingw32_sysroot "Merge in additional sysroot packages"
task_depends merge_in_x86_64_w64_mingw32_sysroot
cmd_merge_in_x86_64_w64_mingw32_sysroot()
{
  if [ -d "$FINAL_TOOLCHAIN" ]; then
    chmod -R +w $FINAL_TOOLCHAIN
    rm -rf $FINAL_TOOLCHAIN || panic "Unable to clean out old toolchain"
  fi

  mkdir -p $FINAL_TOOLCHAIN

  log "Populating the toolchain"
  cp -r $INTER_TOOLCHAIN/$OPT_CROSSTOOL_CONFIG/* $FINAL_TOOLCHAIN

  # Make the sysroot readable
  chmod -R +w $FINAL_TOOLCHAIN
}

# Setup the package dependencies based upon the run config.
DEPENDS1="merge_in_${RUN_CONFIG}_sysroot"
DEPENDS2="build_${RUN_CONFIG}_toolchain"
task_define package_toolchain "Package final toolchain"
task_depends package_toolchain $DEPENDS1 $DEPENDS2
cmd_package_toolchain ()
{
    SRC_DIR=$(dirname "$0")
    DEST=$FINAL_TOOLCHAIN/creation_scripts
    chmod a+w $FINAL_TOOLCHAIN/$OPT_CROSSTOOL_CONFIG
    mkdir -p $DEST/utils

    # Copy this script to the install directory
    cp -f $0 $DEST || panic "Could not copy build script to install directory"
    cp $SRC_DIR/utils/common.shi $DEST/utils || panic "Could not copy build script to install directory"
    cp $SRC_DIR/utils/aosp_dir.shi $DEST/utils || panic "Could not copy build script to install directory"
    cp $SRC_DIR/utils/option_parser.shi $DEST/utils || panic "Could not copy build script to install directory"
    cp $SRC_DIR/utils/temp_dir.shi $DEST/utils || panic "Could not copy build script to install directory"
    cp $SRC_DIR/utils/task.shi $DEST/utils || panic "Could not copy build script to install directory"

    # Package everything
    log "Packing archive $TOOLCHAIN_ARCHIVE with $FINAL_TOOLCHAIN"
    pack_archive $TOOLCHAIN_ARCHIVE "$FINAL_TOOLCHAIN" "*"
}


task_define build_x86_64_w64_mingw32_toolchain "Build the ubuntu toolchain"
task_depends build_x86_64_w64_mingw32_toolchain compile_crosstool
cmd_build_x86_64_w64_mingw32_toolchain ()
{
  cd $BUILD_DIR/$CROSSTOOL_DIR

  CONFIG=samples/$OPT_CROSSTOOL_CONFIG/crosstool.config
  # First we need to fix up our config.
  echo "CT_WANTS_STATIC_LINK=y" >> $CONFIG
  echo "CT_WANTS_STATIC_LINK_CXX=y" >> $CONFIG
  echo "CT_STATIC_TOOLCHAIN=y" >> $CONFIG

  CT_PREFIX=$INTER_TOOLCHAIN ./ct-ng $OPT_CROSSTOOL_CONFIG || panic "Unable to prepare toolcahin"
  CT_PREFIX=$INTER_TOOLCHAIN ./ct-ng build.$NUM_JOBS || panic "Unable to build toolchain"
}

MAIN_TASK=package_toolchain

echo "Running $MAIN_TASK for $OPT_CROSSTOOL_CONFIG"
run_task $MAIN_TASK
echo "Done creating your toolchain. You can find it here: $TOOLCHAIN_ARCHIVE"
