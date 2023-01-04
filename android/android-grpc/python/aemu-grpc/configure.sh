#!/bin/sh
# Copyright 2020 The Android Open Source Project
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
if [ "${BASH_SOURCE-}" = "$0" ]; then
  echo "You must source this script: \$ source $0" >&2
  echo "It will create a virtual environment in which emu-docker will be installed."
  exit 33
fi

PYTHON=python3
PY_VER=$(PYTHON --version)

write_local_pip_conf() {
  cp ./cfg/pip.conf $VIRTUAL_ENV/pip.conf
  cp ./cfg/pypirc $VIRTUAL_ENV/pypirc
}

echo "Using ${PYTHON} which is version ${PY_VER}"
echo "Make sure you have devpi running on port 3141 i.e. run: \$AOSP/external/adt-infra/devpi/launch-devpi.sh"
if [ ! -f "./.venv/bin/activate" ]; then
  $PYTHON -m venv .venv
  write_local_pip_conf
fi

if [ -e ./.venv/bin/activate ]; then
  . ./.venv/bin/activate
  pip install --upgrade pip setuptools wheel
  pip install -e .\[test\]
fi
