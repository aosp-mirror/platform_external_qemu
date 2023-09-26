#!/bin/bash
# This script can be used to bisect the failure of a cts test.
# it is designed to find the cl that introduced a breaking bluetooth change
#
# The script requires the following:
#
# Have 2 avds defined: 34 and 34-2. They should use
# system-image android-34/google_apis
#
# Have the ctsverifier apk in $HOME/Downloads/cts-bluetooth
# Have a current build of the emulator + netsimd
#
# After a while you should have 2 emulators up and running, and you can do the
# cts verifier test of interest. Once you are done and know if it is a good or
# bad build you can close both emulators.
# Next you will have to say Y/N in the terminal to indicate if the build is good
# or bad.
#
#
# Note: This is a very slow activity, so you will likely have tokens expired.
# If that happens just re-run emu-bisect with the --good and --bad flags of
# the last known builds.


# This is the architecture we are testing
aarch=x86_64

# Location of the cts_verifier_apk, set this to where you have it available
cts_verifier_apk=$HOME/Downloads/cts-bluetooth/CtsVerifier.apk

# Location of the emulator build you wish to use for launch
emulator_exe=$HOME/src/emu-master-dev/external/qemu/objs/emulator

# Location of the netsimd binary
netsimd_exe=$HOME/src/emu-master-dev/external/qemu/objs/netsimd
avd1="34"
avd2="34-2"


panic () {
  echo "ERROR: $@" >&2
  exit 1
}

[ ! -f "$cts_verifier_apk" ] && panic "File $cts_verifier_apk does not exist"
[ ! -f "$emulator_exe" ] && panic "File $emulator_exe does not exist"
[ ! -f "$netsimd_exe" ] && panic "File $netsimd_exe does not exist"
set -x

# This configures the device for cts, it will set all the required properties
# and install the apk.
# Param #1 the serial of the device
function install_cts() {
  local device=$1
  adb -s $device shell settings put global hidden_api_policy 1
  adb -s $device install -r -g $cts_verifier_apk
  adb -s $device shell appops set com.android.cts.verifier android:read_device_identifiers allow
  adb -s $device shell appops set com.android.cts.verifier MANAGE_EXTERNAL_STORAGE 0
}

function launch_cts() {
  local device=$1
  adb -s $device shell am start -n com.android.cts.verifier/.CtsVerifierActivity
}

# Prepares the system image by copying it to the system_image directory
# it will use symlinks so you can quickly switch versions.
function prepare_system_image() {
  local build_id=$1
  local system_image_src_zip=$2
  local system_image_dest_dir=$3
  # rm -rf $system_image_dest_dir/$build_id
  mkdir -p $system_image_dest_dir/$build_id
  unzip -n $system_image_src_zip -d $system_image_dest_dir/$build_id
  unlink $system_image_dest_dir/$aarch
  ln -sf  $system_image_dest_dir/$build_id/$aarch $system_image_dest_dir/$aarch
}

# Waits until the device is booted
function wait_for_device_boot() {
  local device=$1
  adb -s $device wait-for-device shell 'while [[ -z $(getprop sys.boot_completed) ]]; do sleep 1; done'
}

# Starts logcat
function logcat() {
  local device=$1
  adb -s $device logcat > /tmp/android-$USER/$device-logcat.log &
}

# device_pid is a global variable used to communicate the pid of the emulator
# proc
device_pid=""
function prepare_device() {
   local avd=$1
   local emulator_bin=$2
   local device=$3

   $emulator_bin -avd $1 -wipe-data &
   pid=$!

  # Wait a bit so we at least have the adb service up and
  # running (we usually don't boot within 10 sec)
  sleep 10

   wait_for_device_boot $device
   logcat $device
   install_cts $device
   launch_cts $device

   echo "Emulator $device ($avd) ready under $pid"
   device_pid="$pid"
}


# Prep the environment by launching netsimd with pcap capture
function prepare_environment() {
  rm -rf /tmp/android-$USER
  $netsimd_exe -d --pcap &
}

# Collect all the left over data.
function collect_data() {
  local dest=$1
  zip -r $dest /tmp/android-$USER

}

prepare_system_image $X $ARTIFACT_LOCATION $ANDROID_SDK_ROOT/system-images/android-34/google_apis/
prepare_environment
prepare_device $avd1 $emulator_exe emulator-5554
pid1=$device_pid

echo "Running emulator under $pid1"
prepare_device $avd2 $emulator_exe emulator-5556
pid2=$device_pid

# Wait until the emulators have finished
wait $pid1 $pid2

# Just kill it all.. it's fine..
pkill -9 netsimd
pkill -9 adb

# Now you as a user must say if it is ok or not!
set +x
read -p "Is $X OK? (y/n): " ok < /dev/tty;

# Collect data, with y-n marker for future analysis
collect_data $HOME/Downloads/test-$X-$ok.zip

# result needed for bisect
[[ "$ok" == "y" ]]
