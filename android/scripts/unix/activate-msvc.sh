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

PROGRAM_DESCRIPTION=\
"Installs the windows sdk in /mnt/msvc so you can start cross building windows MSVC."

DEFINE_string ciopfs "/mnt/ciopfs" "Source location of the case insensitive file system, this will contain the win sdk source. An overlay user mounted filesystem will be created on top of this."

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

function fetch_dependencies_msvc() {
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

    # Download the windows sdk
    local DEPOT_TOOLS="android/third_party/chromium/depot_tools"
    # This is the hash of the msvc version we are using (VS 2017)
    local MSVC_HASH="3bc0ec615cf20ee342f3bc29bc991b5ad66d8d2c"

    [[ -d $DEPOT_TOOLS ]] || gbash::die "Cannot find depot tools: $DEPOT_TOOLS, are you running the script for {aosp}/external/qemu ?"

    LOG INFO "User mounting $DATA_POINT under $MOUNT_POINT"
    $CIOPFS -o use_ino $DATA_POINT $MOUNT_POINT
    LOG INFO "Looking to get tools version: ${MSVC_HASH}"
    LOG INFO "Downloading and copying the SDK (this might take a couple of minutes ...)"
    ${DEPOT_TOOLS}/win_toolchain/get_toolchain_if_necessary.py --force --toolchain-dir=$MOUNT_POINT $MSVC_HASH
    if [ $? -ne 0 ]; then
      echo "${RED}Likely not authenticated... Trying to authenticate${RESET}"
      echo "${RED}Please follow the instructions below. ${RESET}"
      echo "${RED}Your project code is 0.${RESET}"
      
      
      ${DEPOT_TOOLS}/download_from_google_storage --config
      LOG INFO "Downloading and copying the SDK (this might take a couple of minutes ...)"
      ${DEPOT_TOOLS}/win_toolchain/get_toolchain_if_necessary.py --force --toolchain-dir=$MOUNT_POINT $MSVC_HASH || gbash::die "Unable to fetch toolchain"
    fi

    # Setup the symlink to a well known location for the build system.
    ln -sf $MOUNT_POINT/vs_files/$MSVC_HASH $MOUNT_POINT/win8sdk
}


function config_fstab() {
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


echo "This script will install windows sdk in ${FLAGS_ciopfs} and create a mount point in /mnt/msvc"
echo "It will install androids version of ciopfs and will suggest you modify fstab to automount /mnt/msvc"
echo "Note that getting the sdk is likely an interactive process, as you will need to obtain some permissions."
echo "${RED}Install of ciopfs requires sudo privileges"
echo "You will still need to modify fstab yourself, or mount the filesystem manually under /mnt/msvc${RESET}"
approve

fetch_dependencies_msvc "/mnt/msvc" ${FLAGS_ciopfs}
config_fstab ${FLAGS_ciopfs}
