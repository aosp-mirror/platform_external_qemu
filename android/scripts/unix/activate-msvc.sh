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
source gbash.sh || (echo "This script is only supported/needed in the Google environment" && exit 1)
source module lib/colors.sh
PROGDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
export PATH=$PATH:$PROGDIR/../../third_party/chromium/depot_tools

PROGRAM_DESCRIPTION=\
"Installs the windows sdk in /mnt/msvc so you can start cross building windows MSVC."

DEFINE_enum fs --enum="jfs,ciopfs" "jfs" "Underlying filesystem used to provide case insensitive file system."
DEFINE_string ciopfs "/mnt/ciopfs" "Source location of the case insensitive file system, this will contain the win sdk source. An overlay user mounted filesystem will be created on top of this."
DEFINE_string jfs  "/mnt/windows-sdk.img" "The jfs image that will contain the win sdk source. A loopback filesystem will be mounted on top of this"

gbash::init_google "$@"

RED=$(colors::get_color red)
GREEN=$(colors::get_color green)
RESET=$(colors::get_color endcolor)

if [ "${FLAGS_ciopfs}" == "/mnt/msvc" ]
  then warn "This cannot point to /mnt/msvc as this will be used for the overlay."
  exit
fi

function approve() {
    read -p "${GREEN}I know what I'm doing..[Y/N]?${RESET}" -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]
    then
        gbash::die "Ok bye!" 1
    fi
}

function download_sdk() {
    local MOUNT_POINT=$1

    echo "Downloading to $MOUNT_POINT"

    # Download the windows sdk
    # local DEPOT_TOOLS="android/third_party/chromium/depot_tools"
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /tmp/depot_tools
    local DEPOT_TOOLS="/tmp/depot_tools"
    # This is the hash of the msvc version we are using (VS 2019)
    local MSVC_HASH="20d5f2553f"

    [[ -d $DEPOT_TOOLS ]] || gbash::die "Cannot find depot tools: $DEPOT_TOOLS, are you running the script for {aosp}/external/qemu ?"

    LOG INFO "Looking to get tools version: ${MSVC_HASH}"
    LOG INFO "Downloading and copying the SDK (this might take a couple of minutes ...)"
    rm ${HOME}/.boto.bak
    vpython3 ${DEPOT_TOOLS}/win_toolchain/get_toolchain_if_necessary.py --force --toolchain-dir=$MOUNT_POINT $MSVC_HASH

    # Setup the symlink to a well known location for the build system.
    ln -sf $MOUNT_POINT/vs_files/$MSVC_HASH $MOUNT_POINT/win8sdk
}

function setup_ciopfs() {
    # Takes two parameters:
    # $1: The path to the mount point for ciopfs,
    # $2: The path to the data directory for ciopfs
    # The mount point for ciopfs (case-insensitive)
    local MOUNT_POINT="$1"
    # Where the windows sdk is actually stored in ciopfs
    local DATA_POINT="$2"
    # fuser mount it with this..
    local CIOPFS="../../prebuilts/android-emulator-build/common/ciopfs/linux-x86_64/ciopfs"

    # Wipe out the existing directories..
    echo "${RED}Deleting and then creating (using sudo) ${DATA_POINT}${RESET}"
    approve
    sudo rm -rf ${DATA_POINT}
    sudo umount ${MOUNT_POINT}

    # Create the case-insensitive filesystem
    [[ -d ${MOUNT_POINT} ]] || sudo mkdir -p ${MOUNT_POINT} || LOG WARN "Failed to create ${MOUNT_POINT}"
    [[ -d ${DATA_POINT} ]] || sudo mkdir -p ${DATA_POINT} || LOG WARN "Failed to create ${DATA_POINT}"


    sudo chmod a+rwx ${MOUNT_POINT}
    sudo chmod a+rwx ${DATA_POINT}

    [[ -d ${MOUNT_POINT} ]] || gbash::die "Mount point ${MOUNT_POINT} does not exist."
    [[ -d ${DATA_POINT} ]] || gbash::die "Data source ${DATA_POINT} does not exist."

    LOG INFO "User mounting $DATA_POINT under $MOUNT_POINT"
    $CIOPFS -o use_ino $DATA_POINT $MOUNT_POINT

    download_sdk $MOUNT_POINT
}


function config_fstab_ciopfs() {
    # Where the windows sdk is actually stored in ciopfs
    local DATA_POINT="$1"
    local CIOPFS="../../prebuilts/android-emulator-build/common/ciopfs/linux-x86_64/ciopfs"
    local FSTAB="${DATA_POINT}   /mnt/msvc        ciopfs  allow_other,use_ino 0 0"

    [[ -f $CIOPFS ]] || gbash::die "Cannot find ciopfs: $CIOPFS, are you running the script for {aosp}/external/qemu ?"

    echo "${RED}Copying ciopfs so you can automount at boot. (using sudo) ${DATA_POINT}${RESET}"
    approve
    sudo cp $CIOPFS /usr/bin/ciopfs
    sudo ln -sf /usr/bin/ciopfs /sbin/mount.ciopfs

    echo "add the lines below to /etc/fstab if you wish to automount upon restarts"
    echo "${GREEN}# Automount case insensitive fs for android emulator builds.${RESET}"
    echo "${GREEN}${FSTAB}${RESET}"
}


function config_fstab_jfs() {
    local DATA_POINT="$1"
    local FSTAB="${DATA_POINT}   /mnt/msvc        jfs loop"

    echo "add the lines below to /etc/fstab if you wish to automount upon restarts"
    echo "${GREEN}# Automount case insensitive fs for android emulator builds.${RESET}"
    echo "${GREEN}${FSTAB}${RESET}"
}

function setup_jfs() {
    # Takes two parameters:
    # $1: The disk location of the file system
    local DISK_LOC="$1"

    # Make sure we have JFS
    sudo apt-get install -y jfsutils
    sudo mkdir -p /mnt/msvc

    # Create a OS/2 case insensitive loopback fs.
    echo "Creating empty img in /tmp/jfs.img"
    dd if=/dev/zero of=/tmp/jfs.img bs=1M count=8192
    chmod a+rwx /tmp/jfs.img
    echo "Making filesystem"
    mkfs.jfs -q -O /tmp/jfs.img
    sudo mv /tmp/jfs.img ${DISK_LOC}

    # Mount it, and download all the things!
    sudo mount ${DISK_LOC} /mnt/msvc -t jfs -o loop
    sudo chmod a+rwx /mnt/msvc
    download_sdk /mnt/msvc
}

echo "This script will install windows sdk in ${FLAGS_ciopfs} and create a mount point in /mnt/msvc"
echo "It will install jfs, or androids version of ciopfs and will suggest you modify fstab to automount /mnt/msvc"
echo "Note that getting the sdk is likely an interactive process, as you will need to obtain some permissions."
echo "${RED}Installation requires sudo privileges"
echo "You will still need to modify fstab yourself, or mount the filesystem manually under /mnt/msvc upon reboots${RESET}"
echo ""
approve

sudo umount /mnt/msvc
if [ "${FLAGS_fs}" == "jfs" ]; then
   setup_jfs ${FLAGS_jfs}
   config_fstab_jfs ${FLAGS_jfs}
else
  echo "${RED}WARNING! Users have reported concurrency issues using ciopfs, it is recommended to use jfs instead!${RESET}"
  approve
  setup_ciopfs "/mnt/msvc" ${FLAGS_ciopfs}
  config_fstab_ciopfs ${FLAGS_ciopfs}
fi
