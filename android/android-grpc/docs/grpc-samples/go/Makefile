# Go parameters
GOCMD=go
GOBUILD=$(GOCMD) build
GOCLEAN=$(GOCMD) clean
GOTEST=$(GOCMD) test
GOGET=$(GOCMD) get
GOPATH=$(shell go env GOPATH)
BINARY_NAME=httpbridge
BINARY_UNIX=$(BINARY_NAME)_unix
GOPATH = $(shell $(GOCMD) env GOPATH)
export PATH :=$(GOPATH)/bin:$(PATH)
PROTOC=protoc
GRPC_GO_PLUGIN = grpc
GRPC_GO_PLUGIN_PATH ?= `which $(GRPC_GO_PLUGIN)`
PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_GO_PLUGIN)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
SYSTEM_OK = true
endif

MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))
QEMU_ROOT := $(abspath $(MAKEFILE_PATH)/../../../../../..)


all: build

protoc: deps system-check $(QEMU_ROOT)/android/android-grpc/android/emulation/control/emulator_controller.proto
		mkdir -p $(GOPATH)/src/android_emulation_control
		$(PROTOC)  -I/usr/local/include -I$(QEMU_ROOT) \
				-I$(GOPATH)/src \
				-I$(GOPATH)/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
				--go_out=plugins=$(GRPC_GO_PLUGIN):/tmp \
				$(QEMU_ROOT)/android/android-grpc/android/emulation/control/emulator_controller.proto
		$(PROTOC)  -I/usr/local/include -I$(QEMU_ROOT) -I$(GOPATH)/src \
				-I$(GOPATH)/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
				--grpc-gateway_out=logtostderr=true,grpc_api_configuration=$(QEMU_ROOT)/android/android-grpc/android/emulation/control/api_config.yaml:/tmp \
				$(QEMU_ROOT)/android/android-grpc/android/emulation/control/emulator_controller.proto
		$(PROTOC) -I/usr/local/include -I$(QEMU_ROOT) -I$(GOPATH)/src \
				-I$(GOPATH)/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
				--swagger_out=logtostderr=true:. \
				$(QEMU_ROOT)/android/android-grpc/android/emulation/control/emulator_controller.proto
		mv  /tmp/android/android-grpc/android/emulation/control/* $(GOPATH)/src/android_emulation_control

build: protoc
		$(GOBUILD) -o $(BINARY_NAME) -v
test:
		$(GOTEST) -v ./...
clean:
		$(GOCLEAN)
		rm -f $(BINARY_NAME)
		rm -f $(BINARY_UNIX)
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
	@echo "You don't have protoc 3.0.0 installed in your path."
	@echo "Please install Google protocol buffers 3.0.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/google/protobuf/releases/tag/v3.0.0-beta-3.3"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
endif
ifneq ($(SYSTEM_OK),true)
	@false
endif

# Cross compilation
build-linux:
		CGO_ENABLED=0 GOOS=linux GOARCH=amd64 $(GOBUILD) -o $(BINARY_UNIX) -v
