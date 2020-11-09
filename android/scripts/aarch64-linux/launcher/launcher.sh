#!/bin/bash

if [ "$#" -ne 3 ]; then
echo "Usage: $0 rundir  emulator.zip  image.zip"
exit 1
fi

RUNROOT=$1
EMUZIP=$2
IMGZIP=$3

set -e

./setupsdk.sh $RUNROOT $EMUZIP $IMGZIP

./setupdeps.sh $RUNROOT

source $RUNROOT/env.rc

./runemu.sh
