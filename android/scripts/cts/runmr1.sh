#!/bin/bash

# This is to run cts on oc-mr1-emu-dev/release branch only
# and it may not work on other branches.

# Assume there are multiple files: emulator.zip, image.zip
# build-tools.zip, platform-tools.zip platforms.zip
# ./runmr1.sh

topdir=$PWD

# create the sdk skeleton so emulator does not complain

#exit upon error: those files have to be there
set -e

rm -rf runctsdir/sdkhome
mkdir -p runctsdir/sdkroot
mkdir -p runctsdir/sdkhome
mkdir -p runctsdir/sdkroot/system-images/android-27/google_apis_playstore
unzip -o platform-tools.zip -d runctsdir/sdkroot
unzip -o build-tools.zip -d runctsdir/sdkroot
unzip -o emulator.zip -d runctsdir/sdkroot
unzip -o platforms.zip -d runctsdir/sdkroot

if [ ! -d runctsdir/sdkroot/system-images/android-27/google_apis_playstore/x86 ]; then 
unzip image.zip -d runctsdir/sdkroot/system-images/android-27/google_apis_playstore
fi

if [ ! -d runctsdir/android-cts ]; then
unzip android-cts.zip -d runctsdir
fi

if [ ! -d $topdir/runctsdir/android-cts-media-1.4 ]; then
unzip android-cts-media-1.4.zip -d runctsdir
mkdir -p /tmp/android-cts-media
ln -sf $topdir/runctsdir/android-cts-media-1.4 /tmp/android-cts-media/android-cts-media-1.4
fi

# create avds: multiple avds are preferred for first run
# as cts is capable of useing --shards <n>; not suitable
# for retry (cts BUG !!!)
function create_avd {
    name=$1
    mkdir -p $topdir/runctsdir/sdkhome/.android/avd/${name}.avd
    avdininame=$topdir/runctsdir/sdkhome/.android/avd/${name}.ini
    avdconfigname=$topdir/runctsdir/sdkhome/.android/avd/${name}.avd/config.ini
    echo "avd.ini.encoding=UTF-8" >> $avdininame
    echo "path=${topdir}/runctsdir/sdkhome/.android/avd/${name}.avd" >> $avdininame
    echo "path.rel=avd/${cts}.avd" >> $avdininame
    echo "target=android-27" >> $avdininame
cat <<EOT >> $avdconfigname
avd.ini.encoding=UTF-8
abi.type=x86
disk.dataPartition.size=4G
hw.accelerometer=yes
hw.audioInput=yes
hw.battery=yes
hw.camera.back=emulated
hw.camera.front=none
hw.cpu.arch=x86
hw.cpu.ncore=4
hw.dPad=no
hw.device.hash2=MD5:2fa0e16c8cceb7d385183284107c0c88
hw.device.manufacturer=Google
hw.device.name=Nexus 5
hw.gps=yes
hw.gpu.enabled=yes
hw.keyboard=yes
hw.lcd.density=480
hw.lcd.depth=16
hw.mainKeys=no
hw.ramSize=2048
hw.sdCard=no
hw.sensors.orientation=yes
hw.sensors.proximity=yes
hw.trackBall=no
image.sysdir.1=system-images/android-27/google_apis_playstore/x86/
skin.dynamic=no
skin.name=1080x1920
skin.path=1080x1920
tag.display=Google Play
tag.id=google_apis_playstore
vm.heapSize=64
EOT

}

create_avd "cts1"
create_avd "cts2"

#set environment variables
export PATH=$topdir/runctsdir/sdkroot/emulator:$topdir/runctsdir/sdkroot/platform-tools:$PATH
export PATH=$topdir/runctsdir/sdkroot/build-tools:$PATH
export ANDROID_SDK_ROOT=$topdir/runctsdir/sdkroot
export ANDROID_SDK_HOME=$topdir/runctsdir/sdkhome

#lunch adb once at least so it creates the public keys
adb kill-server
adb devices

#do a copy of the adbkey to get around the authentication problem
cp -f ~/.android/adbkey* $topdir/runctsdir/sdkhome/.android/

set +e

#clean up all emulators
pkill -9 qemu

# launch n emulators

emulator -list-avds

# launch cts with shards <n>

# launch emulator, wait for it online and prepare copy images
function wait_boot_complete {
    device=$1
    adb -s $device wait-for-device
    while true; do
        sleep 2
        response=`adb -s $device shell getprop dev.bootcomplete`
        if [[ $response == "1" ]]; then
            break;
        fi
    done
}

function launch_avd {
    avdname=$1
    port=$2
    LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libGL.so.1  DISPLAY=:0 emulator -avd $avdname -wipe-data \
    -no-snapshot -gpu host -no-window -no-audio -port $port &
    wait_boot_complete "emulator-$port"
    echo "$avdname is online"
    echo "copy media files"
    (cd $topdir/runctsdir/android-cts-media-1.4/; \
    $topdir/runctsdir/android-cts-media-1.4/copy_images.sh -s "emulator-$port")
}

function run_cts_shards {
    num_shards=$1
#TODO: when --shards work, enable it
#    echo | $topdir/runctsdir/android-cts/tools/cts-tradefed run cts --shards $num_shards
    echo | $topdir/runctsdir/android-cts/tools/cts-tradefed run cts
}

function rerun_cts {
    mytry=$1
    if [ $mytry -gt "-1" ]; then
        echo | $topdir/runctsdir/android-cts/tools/cts-tradefed run cts --retry $mytry
    else
        echo | $topdir/runctsdir/android-cts/tools/cts-tradefed run cts
    fi
}

function get_last_run {
    echo | $topdir/runctsdir/android-cts/tools/cts-tradefed l r | grep 'cts.*emulator.*sdk_gphone' > result_file
    if [ -s result_file ]; then
        cat result_file | sed -e 's/ \+/ /g' | cut -d' ' -f1,2 | tail -1
    else
        echo "-1 -1"
    fi
}

prev_pass="0"

while true; do
last_run=$(get_last_run)
num_runs=`echo $last_run | cut -d' ' -f1`
num_pass=`echo $last_run | cut -d' ' -f2`
if [ "$num_runs" -eq "-1" ]; then
echo "first run"
elif [ "$num_pass" -gt "$prev_pass" ]; then
echo "do again --retry $num_runs"
prev_pass=$num_pass
else
echo "no improvement give up"
break;
fi

launch_avd "cts1" "5554"
rerun_cts "$num_runs"
pkill -9 qemu
sleep 10

done

pkill -9 qemu
# for loop: repeatedly launch emulator, run cts
# as long as we are making progress: not very fun,
# it is just how cts works, as of now.
