#!/bin/bash
# A script for updating prebuilt standalone driver .so and externally visible
# headers.
# Usage: . ./update.sh <g4 client>
CLIENT=$1
GOOGLE3_BASE=/google/src/cloud/$USER/$CLIENT/google3
GOOGLE3_BIN=$GOOGLE3_BASE/blaze-bin
GOOGLE3_GEN=$GOOGLE3_BASE/blaze-genfiles

function build_google3_target {
    local build_target=$1
    local additional_opts=$2
    pushd .
    client_dir=$(p4 g4d -- $CLIENT)
    echo $client_dir 
    cd $client_dir
    blaze build $build_target -c opt --config=loonix --cpu=k8 --compiler='llvm-libcxx' --stripopt=--strip-unneeded $additional_opts
    popd
}

PREBUILT=$PWD
mkdir -p $PWD/models
# build models
# blaze build -c opt platforms/darwinn/code_generator/model_compiler_run
#blaze-bin/platforms/darwinn/code_generator/model_compiler_run --system_config=`pwd`/platforms/darwinn/config/beagle.txt --model="Test1x1Conv" --prefix=$PREBUILT/models/

# Build the reference driver as a standalone .so
build_google3_target //platforms/darwinn/driver:libreference-driver-none.so

# Copy standalone reference driver
cp -f $GOOGLE3_BIN/platforms/darwinn/driver/libreference-driver-none.so \
  ./libreference-driver-none.so

# Build Beagle USB firmware
build_google3_target //third_party/darwinn/driver/usb:usb_latest_firmware

# Copy Beagle USB firmware
cp $GOOGLE3_GEN/third_party/darwinn/driver/usb/usb_latest_firmware.h \
  generated/third_party/darwinn/driver/usb/usb_latest_firmware.h

# Copy prebuilt compiler library.
build_google3_target //platforms/darwinn/code_generator/api/nnapi:libdarwinn_compiler.so.stripped
cp -f $GOOGLE3_BIN/platforms/darwinn/code_generator/api/nnapi/libdarwinn_compiler.so.stripped ./libdarwinn_compiler.so
chmod +w ./libdarwinn_compiler.so

# TODO (ijsung): generate flatbuffer headers in Android.bp
EXTRA_API_HEADERS="\
  $GOOGLE3_GEN/third_party/darwinn/api/executable_generated.h \
  $GOOGLE3_GEN/third_party/darwinn/api/driver_options_generated.h \
"
cp -af $EXTRA_API_HEADERS generated/third_party/darwinn/api/

COMPILER_HEADERS="\
  $GOOGLE3_BASE/platforms/darwinn/code_generator/api/nnapi/compiler.h \
"

mkdir -p include/platforms/darwinn/code_generator/api/nnapi
cp -af $COMPILER_HEADERS include/platforms/darwinn/code_generator/api/nnapi/

# Copy proto files.
PROTOS_MODEL_CONFIG="\
  $GOOGLE3_BASE/platforms/darwinn/model/config/representation.proto \
  $GOOGLE3_BASE/platforms/darwinn/model/config/array.proto \
  $GOOGLE3_BASE/platforms/darwinn/model/config/internal.proto \
"

mkdir -p proto/platforms/darwinn/model/config
cp -af $PROTOS_MODEL_CONFIG  proto/platforms/darwinn/model/config/

# Copy compiler test files.
build_google3_target //platforms/darwinn/code_generator/api/nnapi:embedded_compiler_test_data
cp -af $GOOGLE3_BIN/platforms/darwinn/code_generator/api/nnapi/libembedded_compiler_test_data.a ./

COMPILER_TEST_SRCS="\
  $GOOGLE3_BASE/platforms/darwinn/code_generator/api/nnapi/compiler_test.cc \
  $GOOGLE3_GEN/platforms/darwinn/code_generator/api/nnapi/embedded_compiler_test_vector.h \
"
mkdir -p tests/platforms/darwinn/code_generator/api/nnapi
cp -af $COMPILER_TEST_SRCS tests/platforms/darwinn/code_generator/api/nnapi/

mkdir -p src/platforms/darwinn/model
cp -af $GOOGLE3_BASE/platforms/darwinn/model/layer_builder.cc src/platforms/darwinn/model/
mkdir -p include/platforms/darwinn/model
cp -af $GOOGLE3_BASE/platforms/darwinn/model/layer_builder.h include/platforms/darwinn/model/
cp -af $GOOGLE3_BASE/platforms/darwinn/model/minmax.h include/platforms/darwinn/model/

# Copy all the necessary headers and sources here. This should ideally be done with copybara in the future
cp -rf $GOOGLE3_BASE/third_party/flatbuffers/include/flatbuffers include
cp $GOOGLE3_BASE/third_party/darwinn/api/*.h include/third_party/darwinn/api
cp $GOOGLE3_BASE/third_party/darwinn/port/*.h include/third_party/darwinn/port
cp $GOOGLE3_BASE/third_party/darwinn/port/default/*.h include/third_party/darwinn/port/default
cp $GOOGLE3_BASE/third_party/darwinn/driver/*.h include/third_party/darwinn/driver
cp $GOOGLE3_BASE/platforms/darwinn/model/*.h include/platforms/darwinn/model

cp $GOOGLE3_BASE/third_party/darwinn/api/*.cc src/third_party/darwinn/api
cp $GOOGLE3_BASE/third_party/darwinn/port/*.cc src/third_party/darwinn/port
cp $GOOGLE3_BASE/third_party/darwinn/port/default/*.cc src/third_party/darwinn/port/default
cp $GOOGLE3_BASE/third_party/darwinn/driver/*.cc src/third_party/darwinn/driver
cp $GOOGLE3_BASE/platforms/darwinn/model/*.cc src/platforms/darwinn/model

