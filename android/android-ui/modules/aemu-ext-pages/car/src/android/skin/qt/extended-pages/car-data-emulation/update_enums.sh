#!/bin/bash

# This script extract all the vehicle enums from the header files generated
# by aidl.

VHAL_VERSION=V2
MASTER_PATH=
OUTPUT=vehicle_constants_generated.h

usage() {
  echo "Extract all vehicle enums from generated AIDL header file.\
        You need to first build your local android branch."
  echo "Usage: $0 [OPTIONS]"
  echo "    -v vehicle_hal_version : default value ${VHAL_VERSION}"
  echo "    -m path_to_local_master"
  echo "    -o output file name : default value ${OUTPUT}"
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

if [[ ${MASTER_PATH} == ""  ]]
  then usage
  exit 1
fi

input_dir=${MASTER_PATH}/out/soong/.intermediates\
/hardware/interfaces/automotive/vehicle/aidl/android.hardware.automotive.vehicle-${VHAL_VERSION}\
-ndk-source/gen/include/aidl/android/hardware/automotive/vehicle

if [ ! -d "${input_dir}" ]
  then echo "The given AIDL generated header directory not found at ${input_dir}"
  exit 1
fi



START_PATTERN="enum class"
END_PATTERN="};"


YEAR=$(date +'%Y')

cat > ${OUTPUT} << EOF
// Copyright (C) $YEAR The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

//DO NOT EDIT, USE update_enum.sh TO UPDATE

#pragma once

namespace emulator {
EOF

for input_file in $(ls "${input_dir}"/*.h)
do
  in_enum=0
  # Keep the space from the input.
  IFS=''
  while read line
  do
    # While compiling on Windows, TRY_AGAIN as variable name would conflict with the macro
    # definition at winsock2.h, so rename it to TRYAGAIN.
    line=${line//TRY_AGAIN/TRYAGAIN}
    if [[ ${line} == ${START_PATTERN}* ]]
      then echo ${line} >> ${OUTPUT}
      let in_enum=1
      continue
    fi

    if [[ ${in_enum} == 1 ]]
      then
      if [[ ${line} == ${END_PATTERN}  ]]
        then let in_enum=0
        echo ${line}$'\n' >> ${OUTPUT}
        continue
      fi
      echo "${line}" >> ${OUTPUT}
    fi
  done <"${input_file}"
done

FILE_END="} // namespace emulator"
echo -e ${FILE_END} >> ${OUTPUT}
