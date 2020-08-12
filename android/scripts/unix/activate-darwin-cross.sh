#!/bin/bash
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
set -x
. $(dirname "$0")/utils/common.shi

shell_import utils/option_parser.shi
shell_import utils/temp_dir.shi


export PATH=$PATH:$PROGDIR/../../third_party/chromium/depot_tools

PROGRAM_DESCRIPTION=\
"Installs the darwin sdk in /mnt/darwin so you can start cross building darwin.

 Note that this requires that you have docker, and access to the xcode.xip you wish
 to use if you do not have a tar.gz with the cross compiler.

 you can go to go/xcode to find more information about how to obtain xcode, or obtain
 it directly from Apple: https://developer.apple.com/download/more.

 You must also have access to llvm-9/clang-9. If you do not have those packages update your
 apt repo by following the steps here:

 https://apt.llvm.org/.

 Or if you feel couragous:

 wget https://apt.llvm.org/llvm.sh
 chmod +x llvm.sh
 sudo ./llvm.sh 9
"

OPT_SDK=
option_register_var "--sdk=<path_to_sdk>" OPT_SDK "Path to the sdk you wish to use, generated from https://github.com/tpoechtrager/osxcross. Make sure the filename matches MacSDKxx.xx.sdk.tar.xz"

OPT_CROSS_TAR=
option_register_var "--xcode-tar=<path_to_osxcross.tar.gz>" OPT_CROSS_TAR "Path to a gz with osxcross compiler, (normally construced by the --sdks flag."


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

echo "${RED}This file will build xcode in a docker container, and install the cross tools in /opt/osxcross"
echo "The cross toolchain requires llvm-9 to be installed. if it is not available you might need to follow"
echo "the steps given here: https://apt.llvm.org/"
echo "It will copy over the /opt/osxcross/lib directory to /usr/lib${RESET}"
approve



function build_sdk() {
    run rm -rf /tmp/xcode-build /tmp/osxcross
    run mkdir -p /tmp/xcode-build
    run mkdir -p /tmp/osxcross
    run cp $OPT_SDK /tmp/xcode-build
    run cp $(dirname "$0")/Dockerfile.darwin.cross /tmp/xcode-build/Dockerfile
    run sudo docker build -t osxcross:1015 /tmp/xcode-build
    id=$(docker create osxcross:1015)
    run sudo docker cp $id:/opt/osxcross /tmp/osxcross
    run sudo docker rm -v $id
    run sudo chown -R $USER /tmp/osxcross
    run tar czf $OPT_CROSS_TAR -C /tmp/osxcross osxcross
}


if [ -z $OPT_CROSS_TAR ]; then
    [[ -f $OPT_SDK ]] || panic "You must provide a valid path to mac os sdk, see --help for details"
    OPT_CROSS_TAR=/tmp/osxcross.tar.gz
    build_sdk
fi

# We currently use the clang-9 backend..
run sudo apt-get install -y  llvm-9-dev clang-9 llvm-9
run sudo rm -rf /opt/osxcross/
run sudo mkdir -p /opt/osxcross
run sudo chmod a+rwx /opt/osxcross
run tar xzf ${OPT_CROSS_TAR} -C /opt/osxcross
# Bind clang-9 --> clang
run sudo update-alternatives \
    --install /usr/bin/clang                 clang                  /usr/bin/clang-9     20 \
    --slave   /usr/bin/clang++               clang++                /usr/bin/clang++-9
run sudo update-alternatives \
    --install /usr/lib/llvm              llvm             /usr/lib/llvm-9  20 \
    --slave   /usr/bin/llvm-config       llvm-config      /usr/bin/llvm-config-9  \
    --slave   /usr/bin/llvm-ar           llvm-ar          /usr/bin/llvm-ar-9 \
    --slave   /usr/bin/llvm-as           llvm-as          /usr/bin/llvm-as-9 \
    --slave   /usr/bin/llvm-bcanalyzer   llvm-bcanalyzer  /usr/bin/llvm-bcanalyzer-9 \
    --slave   /usr/bin/llvm-cov          llvm-cov         /usr/bin/llvm-cov-9 \
    --slave   /usr/bin/llvm-diff         llvm-diff        /usr/bin/llvm-diff-9 \
    --slave   /usr/bin/llvm-dis          llvm-dis         /usr/bin/llvm-dis-9 \
    --slave   /usr/bin/llvm-dwarfdump    llvm-dwarfdump   /usr/bin/llvm-dwarfdump-9 \
    --slave   /usr/bin/llvm-extract      llvm-extract     /usr/bin/llvm-extract-9 \
    --slave   /usr/bin/llvm-link         llvm-link        /usr/bin/llvm-link-9 \
    --slave   /usr/bin/llvm-mc           llvm-mc          /usr/bin/llvm-mc-9 \
    --slave   /usr/bin/llvm-mcmarkup     llvm-mcmarkup    /usr/bin/llvm-mcmarkup-9 \
    --slave   /usr/bin/llvm-nm           llvm-nm          /usr/bin/llvm-nm-9 \
    --slave   /usr/bin/llvm-objdump      llvm-objdump     /usr/bin/llvm-objdump-9 \
    --slave   /usr/bin/llvm-ranlib       llvm-ranlib      /usr/bin/llvm-ranlib-9 \
    --slave   /usr/bin/llvm-readobj      llvm-readobj     /usr/bin/llvm-readobj-9 \
    --slave   /usr/bin/llvm-rtdyld       llvm-rtdyld      /usr/bin/llvm-rtdyld-9 \
    --slave   /usr/bin/llvm-size         llvm-size        /usr/bin/llvm-size-9 \
    --slave   /usr/bin/llvm-stress       llvm-stress      /usr/bin/llvm-stress-9 \
    --slave   /usr/bin/llvm-symbolizer   llvm-symbolizer  /usr/bin/llvm-symbolizer-9 \
    --slave   /usr/bin/llvm-tblgen       llvm-tblgen      /usr/bin/llvm-tblgen-9

run sudo rsync -a /opt/osxcross/osxcross/lib/* /usr/lib
run sudo ldconfig
