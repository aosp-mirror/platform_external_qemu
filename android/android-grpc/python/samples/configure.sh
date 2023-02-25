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
  exit 33
fi

PYTHON=python3
PY_VER=$(PYTHON --version)

echo "Using ${PYTHON} which is version ${PY_VER}"
echo "Note: This expects you to be in the emu-master-dev emulator development branch!"

if [ ! -f "./venv/bin/activate" ]; then
  $PYTHON -m venv .venv
fi

if [ -e ./.venv/bin/activate ]; then
  . ./.venv/bin/activate
  pip install wheel ../aemu-grpc --index-url file://$PWD/../../../../../adt-infra/devpi/repo/simple
fi

