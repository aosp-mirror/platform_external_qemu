# Copyright (C) 2019 The Android Open Source Project
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
#
PYTHON=python3
ifeq (, $(shell which $(PYTHON)))
  $(error "PYTHON=$(PYTHON) not found in $(PATH)")
endif

PYTHON_VERSION_MIN=3.5
PYTHON_VERSION=$(shell $(PYTHON) -c 'import sys; print("%d.%d"% sys.version_info[0:2])' )
PYTHON_VERSION_OK=$(shell $(PYTHON) -c 'import sys;\
  print(int(float("%d.%d"% sys.version_info[0:2]) >= $(PYTHON_VERSION_MIN)))' )

ifeq ($(PYTHON_VERSION_OK),0)
  $(error "Need at least Python version $(PYTHON_VERSION_MIN), you have $(PYTHON_VERSION)")
endif

MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR :=  $(abspath $(MAKEFILE_PATH)/..)
QEMU_ROOT := $(abspath $(MAKEFILE_PATH)/../../../../..)

deps:
	pushd $(QEMU_ROOT)/android/android-grpc/python && $(PYTHON) setup.py develop && popd

