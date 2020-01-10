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

shell_import utils/option_parser.shi
shell_import utils/temp_dir.shi


export PATH=$PATH:$PROGDIR/../../third_party/chromium/depot_tools

PROGRAM_DESCRIPTION=\
"Installs the darwin sdk in /mnt/darwin so you can start cross building darwin.

 Note that this requires that you have docker, and access to the xcode.xip you wish
 to use.

 you can go to go/xcode to find more information about how to obtain xcode, or obtain
 it directly from Apple: https://developer.apple.com/download/more.

 This requires that you have docker installed.
"

OPT_XCODE=
option_register_var "--xcode=<path_to_sdk_xip>" OPT_XCODE "Path to Xcode(11.3) 10.15 sdk."

# Fancy colors in the terminal
if [ -t 1 ] ; then
    RED=`tput setaf 1`
    GREEN=`tput setaf 2`
    RESET=`tput sgr0`
else
    RED=
    GREEN=
    RESET=
fi

function approve() {
    read -p "${GREEN}I know what I'm doing..[Y/N]?${RESET}" -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]
    then
        panic "Ok bye!"
    fi
}

option_parse "$@"

[[ -f $OPT_XCODE ]] || panic "You must provide a valid path to xcode.xip, see --help for details"

echo "This file will build xcode in a docker container, and install the cross tools in /opt/osxcross"
echo "It will copy over the /opt/osxcross/lib director to /usr/lib"
approve


# We currently use the clang-7 backend..
run sudo apt-get install -y --no-install-recommends llvm-7 clang-7
run sudo mkdir -p /opt/osxcross
run sudo chmod a+rwx /opt/osxcross
run mkdir -p /tmp/xcode-build
run cp $OPT_XCODE /tmp/xcode-build/xcode.xip
run cp $(dirname "$0")/Dockerfile.darwin.cross /tmp/xcode-build/Dockerfile
run sudo docker build -t osxcross:1015 /tmp/xcode-build
id=$(docker create osxcross:1015)
run sudo docker cp $id:/opt/osxcross /opt/osxcross
run sudo docker rm -v $id
run sudo rsync -a /opt/osxcross/osxcross/lib/* /usr/lib
run sudo ldconfig
