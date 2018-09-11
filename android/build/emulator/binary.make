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

# definitions shared by host_executable.make and host_static_library.make
#

# Sanity check
LOCAL_BITS := $(call local-build-var,BITS)
ifneq (,$(filter-out 32 64,$(LOCAL_BITS)))
    $(error LOCAL_BITS should be defined to either 32 or 64))
endif

$(call local-build-define,CC)
$(call local-build-define,CXX)
$(call local-build-define,AR)
$(call local-build-define,LD)
$(call local-build-define,DUMPSYMS)

LOCAL_CFLAGS := \
    $(call local-build-var,CFLAGS$(LOCAL_BITS)) \
    $(call local-build-var,CFLAGS) \
    $(LOCAL_CFLAGS)

LOCAL_CXXFLAGS := \
    $(call local-build-var,CXXFLAGS$(LOCAL_BITS)) \
    $(call local-build-var,CXXFLAGS) \
    $(LOCAL_CXXFLAGS)

LOCAL_LDFLAGS := \
    $(call local-build-var,LDFLAGS$(LOCAL_BITS)) \
    $(call local-build-var,LDFLAGS) \
    $(LOCAL_LDFLAGS)

LOCAL_LDLIBS := \
    $(LOCAL_LDLIBS) \
    $(call local-build-var,LDLIBS) \
    $(call local-build-var,LDLIBS$(LOCAL_BITS))

# Ensure only one of -m32 or -m64 is being used and place it first.
# This forces target/host to have same bitness, breaking 64-bit only compilers
# on windows. We rely on build system to setup bitness properly.
LOCAL_CFLAGS := \
    -m$(LOCAL_BITS) \
    $(filter-out -m32 -m64, $(LOCAL_CFLAGS))

LOCAL_LDFLAGS := \
    -m$(LOCAL_BITS) \
    $(filter-out -m32 -m64, $(LOCAL_LDFLAGS))

LOCAL_CPP_EXTENSIONS := .cpp .cc .C .cxx .c++
LOCAL_CXX_EXTENSION_PATTERNS := $(foreach pattern,$(LOCAL_CPP_EXTENSIONS),%$(pattern))

# the directory where we're going to place our object files
LOCAL_OBJS_DIR  := $(call local-intermediates-dir)
LOCAL_OBJECTS   :=
LOCAL_C_SOURCES := $(filter  %.c,$(LOCAL_SRC_FILES))
LOCAL_GENERATED_C_SOURCES := $(filter %.c,$(LOCAL_GENERATED_SOURCES))
LOCAL_GENERATED_CXX_SOURCES := \
    $(filter $(LOCAL_CXX_EXTENSION_PATTERNS),\
        $(LOCAL_GENERATED_SOURCES))
LOCAL_CXX_SOURCES := \
    $(filter $(LOCAL_CXX_EXTENSION_PATTERNS),\
        $(LOCAL_SRC_FILES))
LOCAL_OBJC_SOURCES := $(filter %.m,$(LOCAL_SRC_FILES))
LOCAL_OBJCXX_SOURCES := $(filter %.mm,$(LOCAL_SRC_FILES))

LOCAL_CFLAGS := $(strip $(patsubst %,-I%,$(LOCAL_C_INCLUDES)) $(LOCAL_CFLAGS))

# HACK ATTACK: For the Darwin x86 build, we need to add
# '-read_only_relocs suppress' to the linker command to avoid errors.
ifeq ($(BUILD_TARGET_OS)-$(BUILD_TARGET_BITS),darwin-32)
  LOCAL_LDLIBS += -Wl,-read_only_relocs,suppress
endif

$(foreach src,$(LOCAL_C_SOURCES), \
    $(eval $(call compile-c-source,$(src))) \
)

$(foreach src,$(LOCAL_QT_MOC_SRC_FILES), \
    $(eval $(call compile-qt-moc-source,$(src))) \
)

$(foreach src,$(LOCAL_QT_RESOURCES), \
    $(eval $(call compile-qt-resources,$(src))) \
)

# Build dynamic resources only for 64-bit binaries: dynamic resources are the
# same for all bitnesses anyway.
ifeq ($(LOCAL_BITS),64)
    $(foreach src,$(LOCAL_QT_DYNAMIC_RESOURCES), \
        $(eval $(call compile-qt-dynamic-resources,$(src))) \
    )
endif

$(foreach src,$(LOCAL_QT_UI_SRC_FILES), \
    $(eval $(call compile-qt-uic-source,$(src))) \
)

$(foreach src,$(LOCAL_PROTO_SOURCES), \
    $(eval $(call compile-proto-source,$(src))) \
)

$(foreach src,$(LOCAL_GENERATED_C_SOURCES), \
    $(eval $(call compile-generated-c-source,$(src))) \
)

$(foreach src,$(LOCAL_GENERATED_CXX_SOURCES), \
    $(eval $(call compile-generated-cxx-source,$(src))) \
)

$(foreach src,$(LOCAL_CXX_SOURCES), \
    $(eval $(call compile-cxx-source,$(src))) \
    $(eval $(call tidy-cxx-source,$(src))) \
)

$(foreach src,$(LOCAL_OBJC_SOURCES), \
    $(eval $(call compile-objc-source,$(src))) \
)

$(foreach src,$(LOCAL_OBJCXX_SOURCES), \
    $(eval $(call compile-objcxx-source,$(src))) \
)

$(foreach src,$(LOCAL_COPY_COMMON_PREBUILT_RESOURCES), \
    $(eval $(call install-file,$(COMMON_PREBUILTS_DIR)/$(src),$(call local-resource-install-path,$(notdir $(src))))) \
)

$(foreach src,$(LOCAL_COPY_COMMON_TESTDATA), \
    $(eval $(call install-file,$(COMMON_PREBUILTS_DIR)/testdata/$(src),$(call local-testdata-path,$(notdir $(src))))) \
)

$(foreach src,$(LOCAL_COPY_COMMON_TESTDATA_DIRS), \
    $(eval $(call install-dir,$(COMMON_PREBUILTS_DIR)/testdata/$(src),$(call local-testdata-path,$(src)))) \
)

$(foreach src,$(LOCAL_COPY_FILES), \
    $(eval $(call install-file,$(LOCAL_PATH)/$(src),$(call local-install-path,$(src)))) \
)

# Ensure that we build all generated sources before the objects
$(LOCAL_OBJECTS): | $(LOCAL_GENERATED_SOURCES) $(LOCAL_ADDITIONAL_DEPENDENCIES)

LOCAL_OBJECTS += $(LOCAL_PREBUILT_OBJ_FILES)
