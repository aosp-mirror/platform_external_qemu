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

if [ "${FLAGS_ciopfs}" == "/mnt/msvc" ]
  then warn "This cannot point to /mnt/msvc as this will be used for the overlay."
  exit
fi

function approve() {
    read -p "$(colors::get_color green)I read the --help flag, and I trust it will work out. [Y/N]?$(colors::get_color endcolor)" -n 1 -r
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

    # Create the case-insensitive filesystem
    [[ -d ${MOUNT_POINT} ]] || sudo mkdir -p ${MOUNT_POINT} || LOG WARN "Failed to create ${MOUNT_POINT}"
    [[ -d ${DATA_POINT}/clang-intrins ]] || sudo mkdir -p ${DATA_POINT}/clang-intrins || LOG WARN "Failed to create ${DATA_POINT}"


    sudo chmod a+rx ${MOUNT_POINT}
    sudo chmod a+rwx ${DATA_POINT}
    chmod a+rwx ${DATA_POINT}/clang-intrins

    [[ -d ${MOUNT_POINT} ]] || gbash::die "Mount point ${MOUNT_POINT} does not exist."
    [[ -d ${DATA_POINT} ]] || gbash::die "Data source ${DATA_POINT} does not exist."

    # Download the windows sdk
    local DEPOT_TOOLS="android/third_party/chromium/depot_tools"
    # This is the hash of the msvc version we are using (VS 2017)
    local MSVC_HASH="3bc0ec615cf20ee342f3bc29bc991b5ad66d8d2c"


    [[ -d $DEPOT_TOOLS ]] || gbash::die "Cannot find depot tools: $DEPOT_TOOLS, are you running the script for {aosp}/external/qemu ?"

    LOG INFO "Looking to get tools version: ${MSVC_HASH}"
    LOG INFO "Downloading the SDK (this might take a couple of minutes ...)"
    ${DEPOT_TOOLS}/win_toolchain/get_toolchain_if_necessary.py --force --toolchain-dir=$DATA_POINT $MSVC_HASH

}

function config_fstab() {
    # Where the windows sdk is actually stored in ciopfs
    local DATA_POINT="$1"
    local CIOPFS="../../prebuilts/android-emulator-build/common/ciopfs/linux-x86_64/ciopfs"
    local FSTAB="${DATA_POINT}   /mnt/msvc        ciopfs  allow_other,use_ino 0 0"

    [[ -f $CIOPFS ]] || gbash::die "Cannot find ciopfs: $CIOPFS, are you running the script for {aosp}/external/qemu ?"

    LOG INFO "Copying ciopfs to -> /usr/bin and setting mount link"
    sudo cp $CIOPFS /usr/bin/ciopfs
    sudo ln -sf /usr/bin/ciopfs /sbin/mount.ciopfs

    LOG INFO "add this line below to /etc/fstab and run sudo mount -a"
    echo ${FSTAB}
}


echo "This script will install windows sdk in ${FLAGS_ciopfs} and create a mount point in /mnt/msvc"
echo "It will install androids version of ciopfs and will suggest you modify fstab to automount /mnt/msvc"
echo "Note that getting the sdk is likely an interactive process, as you will need to obtain some permissions."
echo "$(colors::get_color red)Install of ciopfs requires sudo privileges"
echo "You will still need to modify fstab yourself, or mount the filesystem manually under /mnt/msvc$(colors::get_color endcolor)"
approve

fetch_dependencies_msvc "/mnt/msvc" ${FLAGS_ciopfs}
config_fstab ${FLAGS_ciopfs}
