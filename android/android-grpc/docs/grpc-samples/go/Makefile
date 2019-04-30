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
GOBUILD=$(GOCMD) build
GOCLEAN=$(GOCMD) clean
GOTEST=$(GOCMD) test
GOGET=$(GOCMD) get
GOPATH=$(shell go env GOPATH)
BINARY_NAME=httpbridge
GOPATH = $(shell $(GOCMD) env GOPATH)
export PATH :=$(GOPATH)/bin:$(PATH)
PROTOC=protoc
GRPC_GO_PLUGIN = grpc
GRPC_GO_PLUGIN_PATH ?= `which $(GRPC_GO_PLUGIN)`
PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD=$(PROTOC) --version | grep -q "libprotoc 3.6" && echo true || echo false
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
HAS_DEP = $(shell which $(DEP) > /dev/null && echo true || echo false)

ifeq ($(HAS_PROTOC),true)
	HAS_VALID_PROTOC := $(shell $(PROTOC_CHECK_CMD))
endif

MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR :=  $(abspath $(MAKEFILE_PATH)/..)
QEMU_ROOT := $(abspath $(MAKEFILE_PATH)/../../../../../..)
PROTODIR = $(CURRENT_DIR)/android_emulation_control
PROTO = $(PROTODIR)/emulator_controller.proto

all: build

$(PROTO):
		mkdir -p $(PROTODIR)
ifeq (,$(wildcard $(QEMU_ROOT)/android/android-grpc/android/emulation/control/emulator_controller.proto))
		cp  $(ANDROID_SDK_ROOT)/emulator/lib/emulator_controller.proto  $(PROTO)
else
		cp  $(QEMU_ROOT)/android/android-grpc/android/emulation/control/emulator_controller.proto  $(PROTO)
endif

protoc: deps $(PROTO) system-check
		@mkdir -p /tmp/proto
		@mkdir -p $(GOPATH)/src/android_emulation_control
		$(PROTOC) -I$(PROTODIR) \
				-I$(GOPATH)/src \
				-I$(GOPATH)/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
				--go_out=plugins=$(GRPC_GO_PLUGIN):/tmp/proto \
				$(PROTO)
		$(PROTOC) -I$(PROTODIR) -I$(GOPATH)/src \
				-I$(GOPATH)/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
				--grpc-gateway_out=logtostderr=true,grpc_api_configuration=api_config.yaml:/tmp/proto \
				$(PROTO)
		$(PROTOC) -I$(PROTODIR) -I$(GOPATH)/src \
				-I$(GOPATH)/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
				--swagger_out=logtostderr=true:. \
				$(PROTO)
		mv  /tmp/proto/* $(GOPATH)/src/android_emulation_control

build: protoc
		$(GOBUILD) -o $(BINARY_NAME) -v
test:
		$(GOTEST) -v ./...
clean:
		$(GOCLEAN)
		rm -f $(BINARY_NAME)

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
