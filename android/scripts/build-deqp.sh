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
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build dEQP against emulator combined guest/host driver."

aosp_dir_register_option
package_builder_register_options

option_parse "$@"

aosp_dir_parse_option

DEQP_BUILD_DIR=$AOSP_DIR/external/deqp/build
OPT_BUILD_DIR=$DEQP_BUILD_DIR

package_builder_process_options deqp

DEQP_DIR=$AOSP_DIR/external/deqp
DEQP_BUILD_DIR=$AOSP_DIR/external/deqp/build
GOLDFISH_OPENGL_DIR=$AOSP_DIR/device/generic/goldfish-opengl
EMU_DIR=$AOSP_DIR/external/qemu
EMU_OUTPUT_DIR=$AOSP_DIR/external/qemu/objs

rm -rf $DEQP_BUILD_DIR
mkdir -p $DEQP_BUILD_DIR

log $LOCAL_HOST_SYSTEMS

CLANG_REV=4679922

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    case $SYSTEM in
        linux*)
            log "LINUX"
            export CC=$AOSP_DIR/prebuilts/clang/host/linux-x86/clang-$CLANG_REV/bin/clang
            export CXX=$AOSP_DIR/prebuilts/clang/host/linux-x86/clang-$CLANG_REV/bin/clang++
            ;;
        darwin*)
            log "DARWIN"
            export CC=$AOSP_DIR/prebuilts/clang/host/darwin-x86/clang-$CLANG_REV/bin/clang
            export CXX=$AOSP_DIR/prebuilts/clang/host/darwin-x86/clang-$CLANG_REV/bin/clang++
            ;;
    esac
done

log "Copying custom dEQP sources for Android Emulator."

cp -r $EMU_DIR/android/scripts/deqp-src-to-copy/* $DEQP_DIR/

log "Fetching dEQP external sources."

cd $DEQP_DIR

python external/fetch_sources.py

cd $DEQP_BUILD_DIR

cmake $DEQP_BUILD_DIR $DEQP_DIR \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DDEQP_TARGET=aemu \
    -DGOLDFISH_OPENGL_INCLUDE_DIR=$GOLDFISH_OPENGL_DIR/system/include \
    -DAEMU_INCLUDE_DIR=$EMU_DIR/android/android-emugl/combined \
    -DAEMU_LIBRARY_DIR=$EMU_OUTPUT_DIR/lib64 \

make -j $NUM_JOBS

log "Linking lib64 dir into gles3 module."

cd $DEQP_BUILD_DIR/modules/gles3
ln -s $EMU_OUTPUT_DIR/lib64 lib64
cd $DEQP_BUILD_DIR/modules/gles31
ln -s $EMU_OUTPUT_DIR/lib64 lib64
cd $DEQP_BUILD_DIR/modules/gles2
ln -s $EMU_OUTPUT_DIR/lib64 lib64
cd $DEQP_BUILD_DIR/modules/egl
ln -s $EMU_OUTPUT_DIR/lib64 lib64
cd $DEQP_BUILD_DIR/external/vulkancts/modules/vulkan
ln -s $EMU_OUTPUT_DIR/lib64 lib64

log "Done building dEQP."