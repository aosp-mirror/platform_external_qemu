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
GOCMD=go
GOGET=$(GOCMD) get
BINARY_NAME=httpbridge
GOPATH = $(shell $(GOCMD) env GOPATH)
export PATH :=$(GOPATH)/bin:$(PATH)

GRPC_GO_PLUGIN = grpc
GRPC_GO_PLUGIN_PATH ?= `which $(GRPC_GO_PLUGIN)`
DEP=dep
PROTOC=protoc
PROTOC_CMD = which $(PROTOC)

# Check that the minimum protocol version >3.6
MIN_PROTOC_VERSION=3.6
PROTOC_VERSION=($(PROTOC_CMD) --version |  sed 's/[[:alpha:]|(|[:space:]]//g'; echo $(MIN_PROTOC_VERSION)) | sort -V | head -n 1
PROTOHEADER=$(shell pkg-config --variable prefix protobuf)/include
PROTOLIB=$(shell pkg-config --variable prefix protobuf)/lib

HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)

ifeq ($(HAS_PROTOC),true)
	HAS_VALID_PROTOC := $(subst $(MIN_PROTOC_VERSION),true,$(shell $(PROTOC_VERSION)))
endif


all: build

MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
QEMU_ROOT := $(abspath $(MAKEFILE_PATH)/../../../../../..)
CURRENT_DIR :=  $(abspath $(MAKEFILE_PATH)/..)
WEBPROXY = $(GOPATH)/bin/grpcwebproxy

# Protobuf settings.
GRPC_ROOT     := $(QEMU_ROOT)/android/android-grpc
PROTODIR      := $(GOPATH)/src/android_emulation_control
PROTOSRCDIR   := $(if $(wildcard $(GRPC_ROOT)/*.proto), $(GRPC_ROOT), $(ANDROID_SDK_ROOT)/emulator/lib)
PROTO_SRC     := $(wildcard $(PROTOSRCDIR)/*.proto)
PROTO_OBJS    := $(addprefix $(PROTODIR)/, $(notdir $(PROTO_SRC:.proto=.pb.go)))
PROXY_OBJS    := $(addprefix $(PROTODIR)/, $(notdir $(PROTO_SRC:.proto=.pb.gw.go)))

$(PROTODIR):
	@mkdir -p $(PROTODIR)

# Protobuf --> go
$(PROTO_OBJS) : $(PROTOSRCDIR)/emulator_controller.proto $(PROTODIR)
		$(PROTOC) -I$(PROTODIR) -I$(GOPATH)/src -I$(PROTOSRCDIR) \
				-I$(GOPATH)/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
				--go_out=plugins=$(GRPC_GO_PLUGIN),import_path=android_emulation_control:$(PROTODIR) \
				$(PROTO_SRC)

# Protobuf --> proxy
$(PROXY_OBJS) : $(PROTOSRCDIR)/emulator_controller.proto $(PROTODIR)
		$(PROTOC) -I$(PROTODIR) -I$(GOPATH)/src -I$(PROTOSRCDIR) \
				-I$(GOPATH)/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
				--grpc-gateway_out=logtostderr=true,import_path=android_emulation_control,grpc_api_configuration=api_config.yaml:$(PROTODIR) \
				$(PROTO_SRC)



build: $(PROTO_OBJS) $(PROXY_OBJS)
		$(GOCMD) build -o $(BINARY_NAME) -v

clean:
		$(GOCMD) clean
		rm -f $(BINARY_NAME)
		rm -rf $(PROTODIR)

run: build
		$(GOBUILD) -o $(BINARY_NAME) -v ./...
		./$(BINARY_NAME)

deps:
		$(GOGET) github.com/grpc-ecosystem/grpc-gateway/protoc-gen-grpc-gateway
		$(GOGET) github.com/grpc-ecosystem/grpc-gateway/protoc-gen-swagger
		$(GOGET) github.com/golang/protobuf/protoc-gen-go
		$(GOGET) google.golang.org/grpc

system-check:
ifneq ($(HAS_VALID_PROTOC),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have protoc 3.6.0 installed in your path."
	@echo "Please install Google protocol buffers 3.6.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/google/protobuf/releases/tag/v3.6.0"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
	@false
endif
ifneq ($(HAS_VALID_PROTOC),true)
	@false
endif
