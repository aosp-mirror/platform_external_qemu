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

option_register_var "--sanitizer=<clang-fsanitizer-arg>" OPT_SANITIZER "Build dEQP with sanitizer(s) enabled"

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
EMU_CUSTOM_DEQP_SRC_DIR=$AOSP_DIR/external/qemu/android/scripts/deqp-src-to-copy
EMU_OUTPUT_DIR=$AOSP_DIR/external/qemu/objs

rm -rf $DEQP_BUILD_DIR
mkdir -p $DEQP_BUILD_DIR

log $LOCAL_HOST_SYSTEMS

CLANG_REV=4679922

HOST_OS="linux"

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    case $SYSTEM in
        linux*)
            log "LINUX"
            export CC=$AOSP_DIR/prebuilts/clang/host/linux-x86/clang-$CLANG_REV/bin/clang
            export CXX=$AOSP_DIR/prebuilts/clang/host/linux-x86/clang-$CLANG_REV/bin/clang++
            HOST_OS="linux"
            ;;
        darwin*)
            log "DARWIN"
            export CC=$AOSP_DIR/prebuilts/clang/host/darwin-x86/clang-$CLANG_REV/bin/clang
            export CXX=$AOSP_DIR/prebuilts/clang/host/darwin-x86/clang-$CLANG_REV/bin/clang++
            HOST_OS="darwin"
            ;;
    esac
done

log "Copying/patching custom dEQP sources for Android Emulator."

cd $DEQP_DIR

cp -r $EMU_CUSTOM_DEQP_SRC_DIR/* .

for f in *.patch; do
    log "Applying patch: $f"
    if git apply $f; then
        log "Successfully applied $f"
    else
        log "WARNING: could not apply $f"
    fi
done

cd $DEQP_BUILD_DIR

SANITIZER=""
if [ "$OPT_SANITIZER" ]; then
SANITIZER="-fsanitize=$OPT_SANITIZER"
fi

cmake $DEQP_BUILD_DIR $DEQP_DIR \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DDEQP_TARGET=aemu \
    -DGOLDFISH_OPENGL_INCLUDE_DIR=$GOLDFISH_OPENGL_DIR/system/include \
    -DAEMU_INCLUDE_DIR=$EMU_DIR/android/android-emugl/combined \
    -DAEMU_LIBRARY_DIR=$EMU_OUTPUT_DIR/lib64 \
    -DAEMU_SANITIZER=$SANITIZER \
    -DCMAKE_TOOLCHAIN_FILE=$EMU_DIR/android/build/cmake/toolchain-$HOST_OS-x86_64.cmake \

make -j $NUM_JOBS

log "Linking lib64/testlib64 dirs into dEQP modules."

cd $DEQP_BUILD_DIR/modules/gles3
ln -s $EMU_OUTPUT_DIR/lib64 lib64
ln -s $EMU_OUTPUT_DIR/testlib64 testlib64
cd $DEQP_BUILD_DIR/modules/gles31
ln -s $EMU_OUTPUT_DIR/lib64 lib64
ln -s $EMU_OUTPUT_DIR/testlib64 testlib64
cd $DEQP_BUILD_DIR/modules/gles2
ln -s $EMU_OUTPUT_DIR/lib64 lib64
ln -s $EMU_OUTPUT_DIR/testlib64 testlib64
cd $DEQP_BUILD_DIR/modules/egl
ln -s $EMU_OUTPUT_DIR/lib64 lib64
ln -s $EMU_OUTPUT_DIR/testlib64 testlib64
cd $DEQP_BUILD_DIR/external/vulkancts/modules/vulkan
ln -s $EMU_OUTPUT_DIR/lib64 lib64
ln -s $EMU_OUTPUT_DIR/testlib64 testlib64

log "Done building dEQP."
