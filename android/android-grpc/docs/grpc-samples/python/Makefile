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
PROTOC=python -m grpc_tools.protoc
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR :=  $(abspath $(MAKEFILE_PATH)/..)
QEMU_ROOT := $(abspath $(MAKEFILE_PATH)/../../../../../..)
PROTODIR = $(CURRENT_DIR)/android_emulation_control
PROTO = $(PROTODIR)/emulator_controller.proto

all: deps

$(PROTO):
		mkdir -p $(PROTODIR)
ifeq (,$(wildcard $(QEMU_ROOT)/android/android-grpc/android/emulation/control/emulator_controller.proto))
		cp  $(ANDROID_SDK_ROOT)/emulator/lib/emulator_controller.proto  $(PROTO)
else
		cp  $(QEMU_ROOT)/android/android-grpc/android/emulation/control/emulator_controller.proto  $(PROTO)
endif

protoc: deps $(PROTO)
		$(PROTOC) -I$(PROTODIR) \
				--python_out=sample --grpc_python_out=sample \
				$(PROTO)
clean:
		rm -f sample/*pb2*

run: protoc
	python -m sample.core


deps:
		pip install --user grpcio
		pip install --user grpcio-tools
