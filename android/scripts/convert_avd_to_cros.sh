#!/bin/sh

# Copyright 2017 The Android Open Source Project
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


usage() {
  echo "Usage: ${0} avd_name"
  exit 1
}

exit_after_failure() {
  echo "FAILED."
  exit 1
}

sanity_check() {
grep -q "hw\.arc" "${AVD_CONFIG_INI}"
  if [ $? -eq 0 ]; then
    echo "AVD '${AVD_NAME}' is already a Chrome OS AVD."
    exit_after_failure
  fi
  grep -q -e '^hw.cpu.arch=x86$' "${AVD_CONFIG_INI}"
  if [ $? -ne 0 ]; then
    echo "AVD '${AVD_NAME}' is not an x86 image"
    exit_after_failure
  fi
  grep -q -e '^image\.sysdir\.1=system-images/chromeos-m[[:digit:]][[:digit:]]/.*/x86/$' \
    "${AVD_CONFIG_INI}"
  if [ $? -ne 0 ]; then
    echo "AVD '${AVD_NAME}' is not a Chrome OS image"
    exit_after_failure
  fi
}

AVD_NAME="$1"
[ -z ${AVD_NAME} ] && usage

if [ -z "${AVD_DATA_ROOT}" ]; then
  AVD_DATA_ROOT="${HOME}/.android/avd"
fi

AVD_CONFIG_INI="${AVD_DATA_ROOT}/${AVD_NAME}.avd/config.ini"
if [ ! -f "${AVD_CONFIG_INI}" ]; then
  echo "AVD config file '${AVD_CONFIG_INI}' does not exist."
  exit_after_failure
fi

echo "Updating '${AVD_CONFIG_INI}'."

sanity_check

# Set Chrome OS mode
echo "hw.arc=yes" >> "${AVD_CONFIG_INI}"
# If the ABI is x86, AVDManager assumes CPU arch is x86 too. Chrome OS requires
# x86_64 for the CPU arch, even if the Android ABI is x86.
sed -i -e 's/^hw.cpu.arch=x86$/hw.cpu.arch=x86_64/g' "${AVD_CONFIG_INI}"
# Disable GPU acceleration, Chrome OS uses software rendering atm.
sed -i -e 's/^hw.gpu.enabled=.*$/hw.gpu.enabled=no/g' "${AVD_CONFIG_INI}"
sed -i -e 's/^hw.gpu.mode=.*$/hw.gpu.mode=off/g' "${AVD_CONFIG_INI}"

echo "Success!"
exit 0
