#!/bin/bash

# This script extract all the vehicle enums from the header files generated
# by hidl.

# TODO: Add the hidl hearder file to the artifacts on build server. so we can
# update to a certain build number.

VHAL_VERSION=2.0
MASTER_PATH=
OUTPUT=vehicle_constants.h

usage() {
  echo "Extract all vehicle enums from generated hidl header file. You need to first build your local android branch."
  echo "Usage: $0 [OPTIONS]"
  echo "    -v vehicle_hal_version : default value $VHAL_VERSION"
  echo "    -m path_to_local_master"
  echo "    -o output file name : default value $OUTPUT"
  echo "    -h print help"
}

while [ $# -gt 0 ]
do
  case "$1" in
  (-v) VHAL_VERSION=$2; shift;;
  (-m) MASTER_PATH=$2; shift;;
  (-o) OUTPUT=$2; shift;;
  (-h)
    usage
    exit 0;;
  (*)
    echo "Unknown option: $1"
    usage
    exit 1;;
  esac
  shift
done

if [[ $MASTER_PATH == ""  ]]
  then usage
  exit 1
fi

input_file=$MASTER_PATH/out/soong/.intermediates/hardware/interfaces/vehicle/$VHAL_VERSION/android.hardware.vehicle@$VHAL_VERSION\_genc++_headers/gen/android/hardware/vehicle/$VHAL_VERSION/types.h

if [ ! -f "$input_file" ]
  then echo "types.h file not found at $input_file"
  exit 1
fi

START_PATTERN="enum class"
END_PATTERN="};"

> $OUTPUT

FILE_HEADER="//DO NOT EDIT, USE update_enum.sh TO UPDATE\n#pragma once\n#include \"android/utils/compiler.h\"\n\nANDROID_BEGIN_HEADER\n\nnamespace emulator{"

FILE_END="} // namespace emulator\nANDROID_END_HEADER"

echo -e $FILE_HEADER >> $OUTPUT

in_enum=0
IFS=''
while read line
do
  if [[ $line == $START_PATTERN* ]]
    then echo $line >> $OUTPUT
    let in_enum=1
    continue
  fi

  if [[ $in_enum == 1 ]]
    then
    if [[ $line == $END_PATTERN  ]]
      then let in_enum=0
      echo $line$'\n' >> $OUTPUT
      continue
    fi
    echo "$line" >> $OUTPUT
  fi
done <"$input_file"

echo -e $FILE_END >> $OUTPUT
