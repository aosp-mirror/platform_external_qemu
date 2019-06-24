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

# Protobuf settings.
GRPC_ROOT     := $(QEMU_ROOT)/android/android-grpc
PROTODIR 	  := $(CURRENT_DIR)/sample
PROTOSRCDIR   := $(if $(wildcard $(GRPC_ROOT)/*.proto), $(GRPC_ROOT), $(ANDROID_SDK_ROOT)/emulator/lib)
PROTO_SRC     := $(wildcard $(PROTOSRCDIR)/*.proto)
PROTO_OBJS    := $(addprefix $(PROTODIR)/, $(notdir $(PROTO_SRC:.proto=_pb2.py)))
PROXY_OBJS    := $(addprefix $(PROTODIR)/, $(notdir $(PROTO_SRC:.proto=.pb2_grpc.py)))

$(PROTODIR):
	@mkdir -p $(PROTODIR)

# Protobuf --> py (note technically this produces 2 files.)
$(PROTODIR)/%_pb2.py  : $(PROTOSRCDIR)/%.proto $(PROTODIR)
	$(PROTOC) -I$(PROTODIR) -I$(PROTOSRCDIR) \
	 		--python_out=sample --grpc_python_out=$(PROTODIR) \
			$<

all: deps protoc

protoc: $(PROTO_OBJS)

clean:
		rm -f $(PROTODIR)/*pb2*

run: protoc
	python -m sample.logcat

deps:
		pip install --user grpcio
		pip install --user grpcio-tools
