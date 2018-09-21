#!/bin/bash
# Run this script by:
# . ./gen_models.sh
#
# Will generate test inputs and expected outputs for re-generating tests in
# vendor/google/darwinn/test/specs/

# TODO (ijsung) make this part of build process
# TODO (ijsung) generate models for different DarwiNN configurations

TEST_PATH=$PWD
g4d -f gen_models_tmp

blaze build -c opt //platforms/darwinn/code_generator:model_compiler_run

MODELS="Test1x1Conv FacenetMobile8BitV1"
for MODEL in $MODELS; do
  mkdir -p $TEST_PATH/$MODEL
  # Reference driver requires including model in the executable.
  blaze-bin/platforms/darwinn/code_generator/model_compiler_run \
    --system_config=`pwd`/platforms/darwinn/config/beagle.txt \
     --model="$MODEL" \
     --prefix="$TEST_PATH/$MODEL/" \
     --include_model_in_executable \
     --dump_output_reference_layout
done;
cd $TEST_PATH
