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

PROTOC=$(PYTHON) -m grpc_tools.protoc
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR :=  $(abspath $(MAKEFILE_PATH)/..)
QEMU_ROOT := $(abspath $(MAKEFILE_PATH)/../../../../..)

# Protobuf settings. If you are running this in the AOSP tree you will want to run ninja install first.
GRPC_ROOT     := $(QEMU_ROOT)/objs/distribution/emulator/lib
PROTODIR      := $(CURRENT_DIR)/sample/proto
PROTOSRCDIR   := $(if $(wildcard $(GRPC_ROOT)/*.proto), $(GRPC_ROOT), $(ANDROID_SDK_ROOT)/emulator/lib)
PROTO_SRC     := $(wildcard $(PROTOSRCDIR)/*.proto)
PROTO_OBJS    := $(addprefix $(PROTODIR)/, $(notdir $(PROTO_SRC:.proto=_pb2.py)))
PROXY_OBJS    := $(addprefix $(PROTODIR)/, $(notdir $(PROTO_SRC:.proto=.pb2_grpc.py)))

$(PROTODIR):
	@mkdir -p $(PROTODIR)

# Protobuf --> py (note technically this produces 2 files.)
$(PROTODIR)/%_pb2.py  : $(PROTOSRCDIR)/%.proto $(PROTODIR)
	touch $(PROTODIR)/__init__.py
	$(PROTOC) -I$(PROTODIR) -I$(PROTOSRCDIR) \
	 		--python_out=$(PROTODIR) --grpc_python_out=$(PROTODIR) \
			$<

all: deps

$(QEMU_ROOT)/objs:
	$(QEMU_ROOT)/android/rebuild --build config --out $(QEMU_ROOT)/objs

$(GRPC_ROOT): $(QEMU_ROOT)/objs
	ninja -C $(QEMU_ROOT)/objs install

$(PROTO_OBJS): $(GRPC_ROOT)

clean:
	rm -f $(PROTODIR)/*pb2*

deps: $(PROTO_OBJS)
	$(PYTHON) -m pip install --user grpcio grpcio-tools pillow
