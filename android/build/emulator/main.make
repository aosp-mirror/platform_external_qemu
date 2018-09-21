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
_BUILD_DEBUG_INFO_DIR := $(BUILD_OBJS_DIR)/build/debug_info

ifeq ($(wildcard $(_BUILD_CONFIG_MAKE)),)
    $(error "The configuration file '$(_BUILD_CONFIG_MAKE)' doesn't exist, please run the 'android/configure.sh' script")
endif

include $(_BUILD_CONFIG_MAKE)
include $(_BUILD_CORE_DIR)/emulator/definitions.make

.PHONY: all libraries executables clean clean-config clean-objs-dir \
        clean-executables clean-libraries

CLEAR_VARS                := $(_BUILD_CORE_DIR)/emulator/clear_vars.make
BUILD_HOST_EXECUTABLE     := $(_BUILD_CORE_DIR)/emulator/host_executable.make
BUILD_CMAKE               := $(_BUILD_CORE_DIR)/emulator/cmake.make
BUILD_HOST_STATIC_LIBRARY := $(_BUILD_CORE_DIR)/emulator/host_static_library.make
BUILD_HOST_SHARED_LIBRARY := $(_BUILD_CORE_DIR)/emulator/host_shared_library.make
PREBUILT_STATIC_LIBRARY   := $(_BUILD_CORE_DIR)/emulator/prebuilt_static_library.make
PREBUILT_SHARED_LIBRARY   := $(_BUILD_CORE_DIR)/emulator/prebuilt_shared_library.make

_BUILD_DEPENDENCY_DIRS :=

all: libraries executables symbols debuginfo
_BUILD_EXECUTABLES :=
_BUILD_SYMBOLS :=
_BUILD_LIBRARIES :=
_BUILD_DEBUG_INFOS :=
_BUILD_TESTS :=
_BUILD_LINT :=
_BUILD_CMAKE_MAKEFILES :=
_BUILD_SYMBOLS :=

clean: clean-intermediates

distclean: clean clean-config

# Force target, used to make sure sub-make runs and can detect
# changes.
force:
	@true

# let's roll
include $(_BUILD_CORE_DIR)/Makefile.top.mk

libraries: $(_BUILD_LIBRARIES)
executables: $(_BUILD_EXECUTABLES)
symbols: $(_BUILD_SYMBOLS)
debuginfo: $(_BUILD_DEBUG_INFOS)
tests: $(_BUILD_TESTS)
lint: $(_BUILD_LINT)
foo: $(_BUILD_FOO)
	@echo "Hi $(_BUILD_FOO)"


# Zip up the generated symbols and errors.
ifeq (true,$(BUILD_GENERATE_SYMBOLS))
SYMBOL_ZIP:=$(call local-symbol-zip-install-path)

symbols: $(SYMBOL_ZIP)
$(SYMBOL_ZIP): $(_BUILD_SYMBOLS)
	@echo "Zipping symbols: $(SYMBOL_ZIP)"
	$(hide) zip -u $(call local-symbol-zip-install-path) $(_BUILD_SYMBOLS) $(_BUILD_SYMBOLS:.sym=.sym.err)
endif


clean-intermediates:
	rm -rf $(BUILD_OBJS_DIR)/intermediates $(_BUILD_EXECUTABLES) \
	    $(_BUILD_LIBRARIES) $(_BUILD_SYMBOLS) $(_BUILD_SYMBOLS_DIR)

clean-config:
	rm -f $(_BUILD_CONFIG_MAKE) $(_BUILD_CONFIG_HOST_H)

# include dependency information
_BUILD_DEPENDENCY_DIRS := $(sort $(_BUILD_DEPENDENCY_DIRS))
-include $(wildcard $(_BUILD_DEPENDENCY_DIRS:%=%/*.d))
