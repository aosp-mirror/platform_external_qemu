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

shell_import utils/option_parser.shi

PROGRAM_DESCRIPTION=\
"This script is used to launch a stand alone emulator using a system-image and
emulator build. It will block until the emulator has completed booting the
image. That is, the property dev.bootcomplete is set.

It will download the android-sdk and install the emulator build and system image
in a temporary directory if no install directory is provided. This enables the
emulator launch to be completely isolated from any other android sdk deployment.

Currently it has the following limitations:
  - Only supports P images "

PROGRAM_PARAMETERS="<system-img.zip> <emulator.zip>"

OPT_LAUNCH_TIMEOUT=240
option_register_var "--timeout=<seconds>" OPT_LAUNCH_TIMEOUT "Maximum time to wait for the emulator to come up"


OPT_LAUNCH_ROOT=
option_register_var "--root=<path>" OPT_LAUNCH_ROOT "Path used to launch from. will use [/tmp/emulator-XXXXXXX] if not specified"

OPT_IMAGE_TYPE=google_apis
option_register_var "--type=<type>" OPT_IMAGE_TYPE "Image type: [default|google_apis|android-tv|android-wear]"

OPT_WIDTH=1080
option_register_var "--width=<int>" OPT_WIDTH "Width of the device"

OPT_HEIGHT=1920
option_register_var "--height=<int>" OPT_HEIGHT "Width of the device"

OPT_DPI=420
option_register_var "--dpi=<int>" OPT_DPI "Dpi of the device"

OPT_LAUNCH_PREFIX=
option_register_var "--launcher=<launcmd>" OPT_LAUNCH_PREFIX "Launcher prefix for the emulator"

OPT_IMAGE_ARCH=x86
option_register_var "--arch=<arch>" OPT_IMAGE_ARCH "Architecture [x86|x86_64]"

OPT_LAUNCH_ID=
option_register_var "--id=<nr>" OPT_LAUNCH_ID "The device number, used to derive the connection ports"

option_parse "$@"

if [ $PARAMETER_COUNT -lt 2 ]; then
  echo "You must provide a system image and emulator build, see --help for details"
  exit 1
fi

OPT_SYSTEM_IMAGE="$PARAMETER_1"
OPT_EMULATOR="$PARAMETER_2"

if [ -z "${OPT_LAUNCH_ROOT}" ]; then
  log2 "Creating temporary directory"
  TMPDIR=$(mktemp -d /tmp/emulator-XXXXXXXXXXX)
else
  TMPDIR=$OPT_LAUNCH_ROOT
fi


# Best effort to see how many emulators we have running
if [ -z "${OPT_LAUNCH_ID}" ]; then
  OPT_LAUNCH_ID=$(pgrep qemu-system | wc -l)
fi

test -f "${OPT_SYSTEM_IMAGE}" || panic "Cannot find ${OPT_SYSTEM_IMAGE}"
test -f "${OPT_EMULATOR}" || panic "Cannot find ${OPT_EMULATOR}"

export ANDROID_AVD_HOME=${TMPDIR}/android_home
export ANDROID_SDK_ROOT=${TMPDIR}/sdk_root

# We need to know where it is all happening
dump "Using ANDROID_AVD_HOME=${ANDROID_AVD_HOME} and ANDROID_SDK_ROOT=${ANDROID_SDK_ROOT}"

# Our executables
EMULATOR="${ANDROID_SDK_ROOT}/emulator/emulator"


# Create directories
create_directories() {
  mkdir -p "${ANDROID_AVD_HOME}"

  # Now the emulator thinks this will be a valid sdk root.
  mkdir -p "${ANDROID_SDK_ROOT}/platforms"
  mkdir -p "${ANDROID_SDK_ROOT}/platform-tools"
}

# Setup the sdk
setup_sdk() {
  # First create necessary directories
  create_directories
  local ZIP="${TMPDIR}/sdk-tools-linux.zip"

  log "Downloading and unpacking the sdk"
  run wget https://dl.google.com/android/repository/sdk-tools-linux-3859397.zip -O ${ZIP} || panic "Failed to download sdk-tools"

  # Extract the dependencies
  run unzip -o "${ZIP}" -d "${ANDROID_SDK_ROOT}" || panic "Failed to extract the sdk"
  run rm "${ZIP}"

  # Install the components from the sdk we need.
  yes | $ANDROID_SDK_ROOT/tools/bin/sdkmanager --licenses > /dev/null
  run $ANDROID_SDK_ROOT/tools/bin/sdkmanager --update
  run $ANDROID_SDK_ROOT/tools/bin/sdkmanager --verbose  "tools" "platform-tools"
}

# Install the emulator
setup_emulator() {
  run unzip -o "${OPT_EMULATOR}" -d "${ANDROID_SDK_ROOT}" || panic "Failed to unpack the emulator zip"
}


create_avd() {
  # Unzip the system-image
  local IMAGE_DIR="${ANDROID_SDK_ROOT}/system-images/android-P/${OPT_IMAGE_TYPE}/"
  mkdir -p ${IMAGE_DIR}
  run unzip -o "${OPT_SYSTEM_IMAGE}" -d "${IMAGE_DIR}" || panic "Failed to unzip system image"

  # Now create the AVD
  local CONFIG_FILE=${ANDROID_AVD_HOME}/gce_emulator.avd/config.ini
  "${ANDROID_SDK_ROOT}/tools/bin/avdmanager" -v create avd -d pixel -b ${OPT_IMAGE_ARCH} -k "system-images;android-P;${OPT_IMAGE_TYPE};${OPT_IMAGE_ARCH}" --force -n gce_emulator --tag "${OPT_IMAGE_TYPE}"
  test -f "${ANDROID_AVD_HOME}/gce_emulator.ini" || panic "No avd constructed!"

  echo hw.lcd.width = ${OPT_WIDTH} >> ${CONFIG_FILE}
  echo hw.lcd.height = ${OPT_HEIGHT} >> ${CONFIG_FILE}
  echo hw.lcd.density = ${OPT_DPI} >> ${CONFIG_FILE}
  echo hw.lcd.depth = 32 >> ${CONFIG_FILE}
}


launch_emulator() {
  local WAIT=0
  local TELNET_PORT=$(( ($OPT_LAUNCH_ID * 2) + 5554))
  local ADB_PORT=$(($TELNET_PORT + 1))
  local ADB="${ANDROID_SDK_ROOT}/platform-tools/adb -s emulator-${TELNET_PORT}"

  log "Launching the emulator from ${ANDROID_SDK_ROOT}"
  log2 "COMMAND ${OPT_LAUNCH_PREFIX} ${EMULATOR} -avd gce_emulator -verbose -show-kernel -ports $TELNET_PORT,$ADB_PORT"
  ${OPT_LAUNCH_PREFIX} ${EMULATOR} -avd gce_emulator -verbose -show-kernel -ports $TELNET_PORT,$ADB_PORT > $TMPDIR/emulator.log &


  log2 "Waiting with $ADB"
  # Block until the emulator is actually booted. TODO: We should make
  # -wait-until-boot not exit the emulator.
  until ($ADB shell getprop dev.bootcomplete 2>&1 | grep 1) || [ $WAIT -gt $OPT_LAUNCH_TIMEOUT ]
  do
    log2 "Still waiting for the emulator."
    WAIT=$(($WAIT + 1))
    sleep 1
  done

  ($ADB shell getprop dev.bootcomplete 2>&1 | grep 1) || panic "Timeout during boot, the emulater might not be running as expected."
}


# Let's make it happen!
setup_sdk && setup_emulator && create_avd && launch_emulator

