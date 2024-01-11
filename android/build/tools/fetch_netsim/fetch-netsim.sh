#!/bin/bash
# Copyright 2024 The Android Open Source Project
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

# Simple wrapper around virtual environment creation and fetch-netsim invocation
if [[ -f ~/go/bin/oauth2l ]]; then
    OAUTHL=~/go/bin/oauth2l
else
    OAUTHL=oauth2l  # Assume it's on the PATH if ~/go/bin/oauth2l doesn't exist
fi

# Now you can use the OAUTHL variable, e.g.,
if [[ -x "$OAUTHL" ]]; then
    echo "using $OAUTHL."
else
    echo ** Make sure you have oauth2l.  See http://go/oauth2l **
    exit
fi


. $(dirname "$0")/configure.sh
. .venv/bin/activate
$OAUTHL reset
fetch-netsim $@