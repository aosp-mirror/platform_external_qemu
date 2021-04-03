#!/bin/bash -e
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used to generate files in the <platform> directories needed to
# build libvpx. Every time libvpx source code is updated run this script.
#
# The script depends on the bpfmt tool, which may need to be built with
# m -j blueprint_tools
#
# For example, from the top of an Android tree:
# $ source build/envsetup.sh
# $ m -j blueprint_tools
# $ external/libvpx/generate_config.sh
#
# And this will update all the config files needed.

export LC_ALL=C
cd $(dirname $0)
BASE_DIR=$(pwd)
LIBVPX_SRC_DIR="libvpx"
LIBVPX_CONFIG_DIR="config"

# Clean files from previous make.
function make_clean {
  make clean > /dev/null
  rm -f libvpx_srcs.txt
}

# Lint a pair of vpx_config.h and vpx_config.asm to make sure they match.
# $1 - Header file directory.
function lint_config {
  # mips does not contain any assembly so the header does not need to be
  # compared to the asm.
  if [[ "$1" != *mips* ]]; then
    $BASE_DIR/lint_config.sh \
      -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
      -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm
  fi
}

# Print the configuration.
# $1 - Header file directory.
function print_config {
  $BASE_DIR/lint_config.sh -p \
    -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
    -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm
}

# Print the configuration from Header file.
# This function is an abridged version of print_config which does not use
# lint_config and it does not require existence of vpx_config.asm.
# $1 - Header file directory.
function print_config_basic {
  combined_config="$(cat $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
                   | grep -E ' +[01] *$')"
  combined_config="$(echo "$combined_config" | grep -v DO1STROUNDING)"
  combined_config="$(echo "$combined_config" | sed 's/[ \t]//g')"
  combined_config="$(echo "$combined_config" | sed 's/.*define//')"
  combined_config="$(echo "$combined_config" | sed 's/0$/=no/')"
  combined_config="$(echo "$combined_config" | sed 's/1$/=yes/')"
  echo "$combined_config" | sort | uniq
}

# Generate *_rtcd.h files.
# $1 - Header file directory.
# $2 - Architecture.
# $3 - Optional - any additional arguments to pass through.
function gen_rtcd_header {
  echo "Generate $LIBVPX_CONFIG_DIR/$1/*_rtcd.h files."

  rm -rf $BASE_DIR/$TEMP_DIR/libvpx.config
  if [[ "$2" == *mips* ]]; then
    print_config_basic $1 > $BASE_DIR/$TEMP_DIR/libvpx.config
  else
    $BASE_DIR/lint_config.sh -p \
      -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
      -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm \
      -o $BASE_DIR/$TEMP_DIR/libvpx.config
  fi

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.pl \
    --arch=$2 \
    --sym=vp8_rtcd $3 \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $BASE_DIR/$LIBVPX_SRC_DIR/vp8/common/rtcd_defs.pl \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vp8_rtcd.h

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.pl \
    --arch=$2 \
    --sym=vp9_rtcd $3 \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $BASE_DIR/$LIBVPX_SRC_DIR/vp9/common/vp9_rtcd_defs.pl \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vp9_rtcd.h

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.pl \
    --arch=$2 \
    --sym=vpx_scale_rtcd $3 \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $BASE_DIR/$LIBVPX_SRC_DIR/vpx_scale/vpx_scale_rtcd.pl \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_scale_rtcd.h

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.pl \
    --arch=$2 \
    --sym=vpx_dsp_rtcd $3 \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $BASE_DIR/$LIBVPX_SRC_DIR/vpx_dsp/vpx_dsp_rtcd_defs.pl \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_dsp_rtcd.h

  rm -rf $BASE_DIR/$TEMP_DIR/libvpx.config
}

# Generate Config files. "--enable-external-build" must be set to skip
# detection of capabilities on specific targets.
# $1 - Header file directory.
# $2 - Config command line.
function gen_config_files {
  ./configure $2 > /dev/null

  # Generate vpx_config.asm. Do not create one for mips.
  if [[ "$1" != *mips* ]]; then
    if [[ "$1" == *x86* ]]; then
      egrep "#define [A-Z0-9_]+ [01]" vpx_config.h \
        | awk '{print "%define " $2 " " $3}' > vpx_config.asm
    else
      egrep "#define [A-Z0-9_]+ [01]" vpx_config.h \
        | awk '{print $2 " EQU " $3}' \
        | perl $BASE_DIR/$LIBVPX_SRC_DIR/build/make/ads2gas.pl > vpx_config.asm
    fi
  fi

  # Generate vpx_version.h
  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/version.sh "$BASE_DIR/$LIBVPX_SRC_DIR" vpx_version.h

  cp vpx_config.* vpx_version.h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1
  make_clean
  rm -rf vpx_config.* vpx_version.h
}

# Generate a text file containing sources for a config
# $1 - Config
function gen_source_list {
  make_clean
  if [[ "$1" = "mips"* ]] || [[ "$1" = "generic" ]]; then
    config=$(print_config_basic $1)
  else
    config=$(print_config $1)
  fi
  make libvpx_srcs.txt target=libs $config > /dev/null
  mv libvpx_srcs.txt libvpx_srcs_$1.txt
}

# Extract a list of C sources from a libvpx_srcs.txt file
# $1 - path to libvpx_srcs.txt
function libvpx_srcs_txt_to_c_srcs {
    grep ".c$" $1 | grep -v "^vpx_config.c$" | awk '$0="\"libvpx/"$0"\","' | sort
}

# Extract a list of ASM sources from a libvpx_srcs.txt file
# $1 - path to libvpx_srcs.txt
function libvpx_srcs_txt_to_asm_srcs {
    grep ".asm$" $1 | awk '$0="\"libvpx/"$0"\","' | sort
}

# Extract a list of converted ASM sources from a libvpx_srcs.txt file
# $1 - path to libvpx_srcs.txt
function libvpx_srcs_txt_to_asm_S_srcs {
    grep ".asm.S$" $1 | awk '$0="\""$0"\","' | sort
}

# Convert a list of sources to a blueprint file containing a variable
# assignment.
# $1 - Config
function gen_bp_srcs {
  (
    varprefix=libvpx_${1//-/_}
    echo "${varprefix}_c_srcs = ["
    libvpx_srcs_txt_to_c_srcs libvpx_srcs_$1.txt
    echo "\"$LIBVPX_CONFIG_DIR/$1/vpx_config.c\","
    echo "]"
    if grep -q ".asm$" libvpx_srcs_$1.txt; then
      echo
      echo "${varprefix}_asm_srcs = ["
      libvpx_srcs_txt_to_asm_srcs libvpx_srcs_$1.txt
      echo "]"
    fi
    echo
  ) > config_$1.bp
}

# Convert a list of sources to a blueprint file containing a variable
# assignment, relative to a reference config.
# $1 - Config
# $2 - Reference config
function gen_bp_srcs_with_excludes {
  (
    varprefix=libvpx_${1//-/_}
    echo "${varprefix}_c_srcs = ["
    comm -23 <(libvpx_srcs_txt_to_c_srcs libvpx_srcs_$1.txt) <(libvpx_srcs_txt_to_c_srcs libvpx_srcs_$2.txt)
    echo "\"$LIBVPX_CONFIG_DIR/$1/vpx_config.c\","
    echo "]"
    echo
    echo "${varprefix}_exclude_c_srcs = ["
    comm -13 <(libvpx_srcs_txt_to_c_srcs libvpx_srcs_$1.txt) <(libvpx_srcs_txt_to_c_srcs libvpx_srcs_$2.txt)
    echo "\"$LIBVPX_CONFIG_DIR/$2/vpx_config.c\","
    echo "]"
    echo
    if grep -qE ".asm(.S)?$" libvpx_srcs_$1.txt; then
      echo
      echo "${varprefix}_asm_srcs = ["
      libvpx_srcs_txt_to_asm_srcs libvpx_srcs_$1.txt
      libvpx_srcs_txt_to_asm_S_srcs libvpx_srcs_$1.txt
      echo "]"
    fi
    echo
  ) > config_$1.bp
}

# The ARM assembly sources must be converted from ADS to GAS compatible format.
# This step is only required for ARM. MIPS uses intrinsics exclusively and x86
# requires 'yasm' to pre-process its assembly files.
function convert_arm_asm {
  find $BASE_DIR/$LIBVPX_CONFIG_DIR/arm-neon -name '*.asm.S' | xargs -r rm
  for src in $(grep ".asm$" libvpx_srcs_arm-neon.txt); do
    newsrc=$LIBVPX_CONFIG_DIR/arm-neon/$src.S
    mkdir -p $BASE_DIR/$(dirname $newsrc)
    perl $BASE_DIR/$LIBVPX_SRC_DIR/build/make/ads2gas.pl <$BASE_DIR/$LIBVPX_SRC_DIR/$src >$BASE_DIR/$newsrc
    echo $newsrc >>libvpx_srcs_arm-neon.txt
  done
  sed -i "/\.asm$/ d" libvpx_srcs_arm-neon.txt
}

echo "Create temporary directory."
TEMP_DIR="$LIBVPX_SRC_DIR.temp"
rm -rf $TEMP_DIR
cp -R $LIBVPX_SRC_DIR $TEMP_DIR
cd $TEMP_DIR

echo "Generate config files."
all_platforms="--enable-external-build --enable-realtime-only --enable-pic"
all_platforms+=" --disable-runtime-cpu-detect --disable-install-docs"
all_platforms+=" --size-limit=4096x3072 --enable-vp9-highbitdepth"
intel="--disable-sse4_1 --disable-avx --disable-avx2 --disable-avx512 --as=yasm"
gen_config_files x86 "--target=x86-linux-gcc ${intel} ${all_platforms}"
gen_config_files x86_64 "--target=x86_64-linux-gcc ${intel} ${all_platforms}"
gen_config_files arm "--target=armv7-linux-gcc --disable-neon ${all_platforms}"
gen_config_files arm-neon "--target=armv7-linux-gcc ${all_platforms}"
gen_config_files arm64 "--force-target=armv8-linux-gcc ${all_platforms}"
gen_config_files mips32 "--target=mips32-linux-gcc --disable-dspr2 --disable-msa ${all_platforms}"
gen_config_files mips32-dspr2 "--target=mips32-linux-gcc --enable-dspr2 ${all_platforms}"
gen_config_files mips32-msa "--target=mips32-linux-gcc --enable-msa ${all_platforms}"
gen_config_files mips64 "--target=mips64-linux-gcc --disable-msa ${all_platforms}"
gen_config_files mips64-msa "--target=mips64-linux-gcc --enable-msa ${all_platforms}"
gen_config_files generic "--target=generic-gnu ${all_platforms}"

echo "Remove temporary directory."
cd $BASE_DIR
rm -rf $TEMP_DIR

echo "Lint libvpx configuration."
lint_config x86
lint_config x86_64
lint_config arm
lint_config arm-neon
lint_config arm64
lint_config mips32
lint_config mips32-dspr2
lint_config mips32-msa
lint_config mips64
lint_config mips64-msa
lint_config generic

echo "Create temporary directory."
TEMP_DIR="$LIBVPX_SRC_DIR.temp"
rm -rf $TEMP_DIR
cp -R $LIBVPX_SRC_DIR $TEMP_DIR
cd $TEMP_DIR

gen_rtcd_header x86 x86 "${intel}"
gen_rtcd_header x86_64 x86_64 "${intel}"
gen_rtcd_header arm armv7 "--disable-neon"
gen_rtcd_header arm-neon armv7
gen_rtcd_header arm64 armv8
gen_rtcd_header mips32 mips32
gen_rtcd_header mips32-dspr2 mips32
gen_rtcd_header mips32-msa mips32
gen_rtcd_header mips64 mips64
gen_rtcd_header mips64-msa mips64
gen_rtcd_header generic generic

echo "Prepare Makefile."
./configure --target=generic-gnu > /dev/null
make_clean

echo "Generate source lists"
gen_source_list x86
gen_source_list x86_64
gen_source_list arm
gen_source_list arm-neon
gen_source_list arm64
gen_source_list mips32
gen_source_list mips32-dspr2
gen_source_list mips32-msa
gen_source_list mips64
gen_source_list mips64-msa
gen_source_list generic

echo "Convert ARM assembly format"
convert_arm_asm

echo "Convert to bp"
gen_bp_srcs x86
gen_bp_srcs x86_64
gen_bp_srcs arm
gen_bp_srcs_with_excludes arm-neon arm
gen_bp_srcs arm64
gen_bp_srcs mips32
gen_bp_srcs_with_excludes mips32-dspr2 mips32
gen_bp_srcs_with_excludes mips32-msa mips32
gen_bp_srcs mips64
gen_bp_srcs_with_excludes mips64-msa mips64
gen_bp_srcs generic

rm -f $BASE_DIR/Android.bp
(
  echo "// THIS FILE IS AUTOGENERATED, DO NOT EDIT"
  echo "// Generated from Android.bp.in, run ./generate_config.sh to regenerate"
  echo
  cat config_*.bp
  cat $BASE_DIR/Android.bp.in
) > $BASE_DIR/Android.bp
bpfmt -w $BASE_DIR/Android.bp

echo "Remove temporary directory."
cd $BASE_DIR
rm -rf $TEMP_DIR
