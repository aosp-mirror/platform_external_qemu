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

# this turns off the suffix rules built into make
.SUFFIXES:

# this turns off the RCS / SCCS implicit rules of GNU Make
% : RCS/%,v
% : RCS/%
% : %,v
% : s.%
% : SCCS/s.%

# If a rule fails, delete $@.
.DELETE_ON_ERROR:

# shared definitions
ifeq ($(strip $(SHOW)),)
define pretty
@echo $1
endef
hide := @
else
define pretty
endef
hide :=
endif

# Return the parent directory of a given path.
# $1: path
parent-dir = $(dir $1)

# Return the directory of the current Makefile / Android.mk.
my-dir = $(call parent-dir,$(lastword $(MAKEFILE_LIST)))

# Return the directory containing the intermediate files for a given
# kind of executable
# $1 = type (EXECUTABLES, STATIC_LIBRARIES or SHARED_LIBRARIES).
# $2 = module name
# $3 = ignored
#
intermediates-dir-for = $(OBJS_DIR)/intermediates/$(2)

# Return the name of a given host tool, based on the value of
# LOCAL_HOST_BUILD. If the variable is defined, return $(BUILD_$1),
# otherwise return $(HOST_$1).
# $1: Tool name (e.g. CC, LD, etc...)
#
local-host-tool = $(if $(strip $(LOCAL_HOST_BUILD)),$(BUILD_$1),$(MY_$1))
local-host-exe = $(call local-host-tool,EXEEXT)
local-host-dll = $(call local-host-tool,DLLEXT)

local-host-define = $(if $(strip $(LOCAL_$1)),,$(eval LOCAL_$1 := $$(call local-host-tool,$1)))

# Return the directory containing the intermediate files for the current
# module. LOCAL_MODULE must be defined before calling this.
local-intermediates-dir = $(OBJS_DIR)/intermediates/$(LOCAL_MODULE)

local-library-path = $(OBJS_DIR)/libs/$(1).a
local-executable-path = $(OBJS_DIR)/$(1)$(call local-host-tool,EXEEXT)
local-shared-library-path = $(OBJS_DIR)/lib/$(1)$(call local-host-tool,DLLEXT)


# Toolchain control support.
# It's possible to switch between the regular toolchain and the host one
# in certain cases.

# Compile a C source file
#
define  compile-c-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(SRC:%.c=%.o)
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CC     := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(LOCAL_PATH)/$$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(LOCAL_PATH)/$$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CC) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

# Compile a C++ source file
#
define  compile-cxx-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(SRC:%$(LOCAL_CPP_EXTENSION)=%.o)
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CXX    := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(LOCAL_PATH)/$$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(LOCAL_PATH)/$$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CXX) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

# Compile an Objective-C source file
#
define  compile-objc-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(notdir $$(SRC:%.m=%.o))
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CC     := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(LOCAL_PATH)/$$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(LOCAL_PATH)/$$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CC) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

# Compile a generated C source files#
#
define compile-generated-c-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(notdir $$(SRC:%.c=%.o))
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CC     := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CC) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

define compile-generated-cxx-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(notdir $$(SRC:%$(LOCAL_CPP_EXTENSION)=%.o))
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CXX    := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CXX) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

# Install a file
#
define install-target
SRC:=$(1)
DST:=$(2)
$$(DST): PRIVATE_SRC := $$(SRC)
$$(DST): PRIVATE_DST := $$(DST)
$$(DST): PRIVATE_DST_NAME := $$(notdir $$(DST))
$$(DST): PRIVATE_SRC_NAME := $$(SRC)
$$(DST): $$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_DST))
	@echo "Install: $$(PRIVATE_DST_NAME) <= $$(PRIVATE_SRC_NAME)"
	$(hide) cp -f $$(PRIVATE_SRC) $$(PRIVATE_DST)
install: $$(DST)
endef

# for now, we only use prebuilt SDL libraries, so copy them
define copy-prebuilt-lib
_SRC := $(1)
_SRC1 := $$(notdir $$(_SRC))
_DST := $$(LIBS_DIR)/$$(_SRC1)
LIBRARIES += $$(_DST)
$$(_DST): PRIVATE_DST := $$(_DST)
$$(_DST): PRIVATE_SRC := $$(_SRC)
$$(_DST): $$(_SRC)
	@mkdir -p $$(dir $$(PRIVATE_DST))
	@echo "Prebuilt: $$(PRIVATE_DST)"
	$(hide) cp -f $$(PRIVATE_SRC) $$(PRIVATE_DST)
endef

define  create-dir
$(1):
	mkdir -p $(1)
endef

define transform-generated-source
@echo "Generated: $(PRIVATE_MODULE) <= $<"
@mkdir -p $(dir $@)
$(hide) $(PRIVATE_CUSTOM_TOOL)
endef
