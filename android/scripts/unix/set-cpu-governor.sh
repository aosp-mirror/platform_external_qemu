#!/bin/sh

# Copyright 2016 The Android Open Source Project
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

# A small script used to set the CPU governor to different values.

# Check parameter
case $1 in
    ondemand|performance|powersave)
        ;;
    *)
        echo >&2 "Please use one of the following parameter: \
powersave, ondemand, performance"
        exit 1
        ;;
esac

# Check that kondemand kernel thread is not running.
if $(pgrep -lf kondemand >/dev/null); then
    echo >&2 "Sorry, kondemand kernel thread is running, cannot change CPU governor"
    return
fi

# Perform the change with sudo
sudo sh -c "\
    for CPUFREQ in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do \
        if [ -f \"\$CPUFREQ\" ]; then \
            echo -n $1 > \$CPUFREQ; \
        fi \
    done \
"
