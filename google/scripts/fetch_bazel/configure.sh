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
  echo "It will create a virtual environment in which fetch-bazel will be installed."
  exit 33
fi

PYTHON=python3
if [ ! -f "./.venv/bin/activate" ]; then
  $PYTHON -m venv .venv
fi

if [ ! -e ./.venv/bin/activate ]; then
  echo "No virtual environment available for activation.."
  echo "Manually run pip install ."
  exit 33
fi

package_dir=$(dirname "$0")
. ./.venv/bin/activate
pip install --upgrade pip setuptools wheel
pip install -e $package_dir\[test\]