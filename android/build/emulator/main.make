# Copyright (C) 2008 The Android Open Source Project
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

# disable implicit rules
.SUFFIXES:
%:: %,v
%:: RCS/%
%:: RCS/%,v
%:: s.%
%:: SCCS/s.%
%.c: %.w %.ch

# this is a set of definitions that allow the usage of Makefile.android
# even if we're not using the Android build system.
#

BUILD_OBJS_DIR := objs

_BUILD_CORE_DIR  := android/build
_BUILD_CONFIG_MAKE := $(BUILD_OBJS_DIR)/build/config.make
_BUILD_CONFIG_HOST_H := $(BUILD_OBJS_DIR)/build/config-host.h
_BUILD_SYMBOLS_DIR := $(BUILD_OBJS_DIR)/build/symbols

ifeq ($(wildcard $(_BUILD_CONFIG_MAKE)),)
    $(error "The configuration file '$(_BUILD_CONFIG_MAKE)' doesn't exist, please run the 'android-configure.sh' script")
endif

include $(_BUILD_CONFIG_MAKE)
include $(_BUILD_CORE_DIR)/emulator/definitions.make

.PHONY: all libraries executables clean clean-config clean-objs-dir \
        clean-executables clean-libraries

CLEAR_VARS                := $(_BUILD_CORE_DIR)/emulator/clear_vars.make
BUILD_HOST_EXECUTABLE     := $(_BUILD_CORE_DIR)/emulator/host_executable.make
BUILD_HOST_STATIC_LIBRARY := $(_BUILD_CORE_DIR)/emulator/host_static_library.make
BUILD_HOST_SHARED_LIBRARY := $(_BUILD_CORE_DIR)/emulator/host_shared_library.make
PREBUILT_STATIC_LIBRARY   := $(_BUILD_CORE_DIR)/emulator/prebuilt_static_library.make

_BUILD_DEPENDENCY_DIRS :=

all: libraries executables symbols
_BUILD_EXECUTABLES :=
_BUILD_SYMBOLS :=
_BUILD_LIBRARIES   :=
_BUILD_INTERMEDIATE_SYMBOLS :=

clean: clean-intermediates

distclean: clean clean-config

# let's roll
include Makefile.top.mk

libraries: $(_BUILD_LIBRARIES)
executables: $(_BUILD_EXECUTABLES)
symbols: $(_BUILD_INTERMEDIATE_SYMBOLS) $(_BUILD_SYMBOLS)

clean-intermediates:
	rm -rf $(BUILD_OBJS_DIR)/intermediates $(_BUILD_EXECUTABLES) \
	    $(_BUILD_LIBRARIES) $(_BUILD_SYMBOLS) $(_BUILD_SYMBOLS_DIR)

clean-config:
	rm -f $(_BUILD_CONFIG_MAKE) $(_BUILD_CONFIG_HOST_H)

# include dependency information
_BUILD_DEPENDENCY_DIRS := $(sort $(_BUILD_DEPENDENCY_DIRS))
-include $(wildcard $(_BUILD_DEPENDENCY_DIRS:%=%/*.d))
