#!/bin/bash

if [ "$#" -ne 3 ]; then
echo "Usage: $0 rundir  emulator.zip  image.zip"
exit 1
fi

RUNROOT=$1
EMU_ZIP=$2
IMG_ZIP=$3
if [ -d $RUNROOT ]; then
echo "$RUNROOT exists, remove it and try again."
exit 2
fi

# error out if anything goes wrong
set -e

# setup sdkroot dirs
mkdir -p $RUNROOT/sdkroot
mkdir -p $RUNROOT/sdkroot/build-tools
mkdir -p $RUNROOT/sdkroot/platform-tools
mkdir -p $RUNROOT/sdkroot/platforms
mkdir -p $RUNROOT/sdkroot/system-images/android-30/default
# unzip $IMG_ZIP -d $RUNROOT/sdkroot/system-images/android-30/default
bsdtar xvf $EMU_ZIP -C $RUNROOT/sdkroot
bsdtar xvf $IMG_ZIP -C $RUNROOT/sdkroot/system-images/android-30/default

# setup sdkhome dirs

mkdir -p $RUNROOT/sdkhome
mkdir -p $RUNROOT/sdkhome/.android/avd
mkdir -p $RUNROOT/sdkhome/.android/avd/a1.avd

topdir=$PWD
#set environment variables
echo "export PATH=$topdir/$RUNROOT/sdkroot/emulator:$topdir/$RUNROOT/sdkroot/platform-tools:\$PATH" > $topdir/$RUNROOT/env.rc
echo "export ANDROID_HOME=$topdir/$RUNROOT/sdkroot" >> $topdir/$RUNROOT/env.rc
echo "export ANDROID_SDK_ROOT=$topdir/$RUNROOT/sdkroot" >> $topdir/$RUNROOT/env.rc
echo "export ANDROID_SDK_HOME=$topdir/$RUNROOT/sdkhome" >> $topdir/$RUNROOT/env.rc

# create the avd with name a1
# config.ini


SKIN="1080x1920x480"
avdWidth=$(echo $SKIN | cut -d'x' -f1)
avdHeight=$(echo $SKIN | cut -d'x' -f2)
avdDPI=$(echo $SKIN | cut -d'x' -f3)

function create_avd {
    name=$1
    ABI=$2
    IMGNAME=$3
    mkdir -p $topdir/$RUNROOT/sdkhome/.android/avd/${name}.avd
    avdininame=$topdir/$RUNROOT/sdkhome/.android/avd/${name}.ini
    avdconfigname=$topdir/$RUNROOT/sdkhome/.android/avd/${name}.avd/config.ini
    echo "avd.ini.encoding=UTF-8" >> $avdininame
    echo "path=${topdir}/${RUNROOT}/sdkhome/.android/avd/${name}.avd" >> $avdininame
    echo "path.rel=avd/${name}.avd" >> $avdininame
    echo "target=android-30" >> $avdininame
cat <<EOT >> $avdconfigname
avd.ini.encoding=UTF-8
abi.type=$ABI
disk.dataPartition.size=2G
hw.accelerometer=yes
hw.audioInput=yes
hw.battery=yes
hw.camera.back=emulated
hw.camera.front=emulated
hw.cpu.arch=$ABI
hw.cpu.ncore=4
hw.dPad=no
hw.device.hash2=MD5:2fa0e16c8cceb7d385183284107c0c88
hw.device.manufacturer=Google
hw.device.name=Nexus 5
hw.gps=yes
hw.gpu.enabled=yes
hw.keyboard=yes
hw.lcd.depth=16
hw.mainKeys=no
hw.ramSize=2048
hw.sdCard=no
hw.sensors.orientation=yes
hw.sensors.proximity=yes
hw.trackBall=no
image.sysdir.1=system-images/android-30/default/$IMGNAME/
skin.dynamic=no
hw.lcd.density=$avdDPI
skin.name=${avdWidth}x${avdHeight}
skin.path=${avdWidth}x${avdHeight}
tag.display=$ABI
tag.id=$ABI
vm.heapSize=64
EOT

}

create_avd "a1" "arm64" "arm64-v8a"

