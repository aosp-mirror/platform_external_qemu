# Copyright (C) 2018 The Android Open Source Project
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
#
# Defines a cmake project, that is it will translate a CMakeLists.txt -> Makefile
# It will setup the toolchain by exporting all the compilers, and archivers.
# Note that we will use CC/CXX to compile and link (as we do everywhere else)
# We currently pass down all the compiler flags and settings.

# first we generate the cmake project
LOCAL_CMAKE_MODULE   :=$(call local-cmake-path,$(LOCAL_MODULE))
LOCAL_CMAKELISTS     :=$(LOCAL_PATH)/CMakeLists.txt
LOCAL_BUILT_MAKEFILE :=$(LOCAL_CMAKE_MODULE)/Makefile

# Next we need to export all the compiler and linker flags,
# We will expand them as usually is done in binary.make

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

# Generate the proper cmake translation target, based upon the host build property
ifeq ($(strip $(LOCAL_HOST_BUILD)),)
 $(eval $(call cmake-project-target,$(LOCAL_CMAKELISTS),$(LOCAL_BUILT_MAKEFILE)))
 $(eval LOCAL_EXEEXT:=$(BUILD_TARGET_EXEEXT))
 $(eval LOCAL_TARGET:=true)
else
 $(eval LOCAL_TARGET:=)
 $(eval $(call cmake-project-host,$(LOCAL_CMAKELISTS),$(LOCAL_BUILT_MAKEFILE)))
 $(eval LOCAL_EXEEXT:=$(BUILD_HOST_EXEEXT))
endif

## Copy dependency data, needed for the unit tests..
$(foreach src,$(LOCAL_COPY_COMMON_PREBUILT_RESOURCES), \
    $(eval $(call install-file,$(COMMON_PREBUILTS_DIR)/$(src),$(call local-resource-install-path,$(notdir $(src))))) \
)

$(foreach src,$(LOCAL_COPY_COMMON_TESTDATA), \
    $(eval $(call install-file,$(COMMON_PREBUILTS_DIR)/testdata/$(src),$(call local-testdata-path,$(notdir $(src))))) \
)

$(foreach src,$(LOCAL_COPY_COMMON_TESTDATA_DIRS), \
    $(eval $(call install-dir,$(COMMON_PREBUILTS_DIR)/testdata/$(src),$(call local-testdata-path,$(src)))) \
)

LOCAL_SOURCE_DEPENDENCIES += $(LOCAL_ADDITIONAL_DEPENDENCIES)

# Note that order of execution is very important, as we are setting up local source
# dependencies between the generated archives.
# We need this in our frankenstein build as we want to make sure that gnumake deals
# properly with concurrency. (If we don't than cmake starts interfering in a nasty way).
#
# Generally it goes like this:
# > Protobufs > archives > shared_libraries > executables > unittests
#
# 1. We compile all the protobufs and make them available.
# 2. We compile the archives, they might use generated protobuf files
# 3. We created shared libs, the could link against archives we created in step 1,2
# 4. The executables, who could depend on 1,2,3..
#
# This works, but has the unfortunate side effect that the second (cmake) build will do
# a NOP depenency analysis (which we want). There is no real easy way around this
# and will be stuck with this until we transition completely.
$(foreach lib,$(PRODUCED_PROTO_LIBS), \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/lib$(lib).a) \
  $(eval LOCAL_FINAL_MODULE := $(call local-library-path,lib$(lib)_proto)) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE),$(lib), $(LOCAL_SOURCE_DEPENDENCIES))) \
  $(eval $(call install-file,$(LOCAL_BUILT_MODULE),$(LOCAL_FINAL_MODULE))) \
  $(eval ALL_PROTOBUF_LIBS += $(BUILD_TARGET_STATIC_PREFIX)$(lib)_proto) \
  $(eval PROTOBUF_DEPS += $(LOCAL_FINAL_MODULE) ) \
  $(eval PROTOBUF_INCLUDES += $(LOCAL_CMAKE_MODULE))\
  $(eval LOCAL_SOURCE_DEPENDENCIES += $(LOCAL_BUILT_MODULE)) \
)

$(foreach lib,$(PRODUCED_STATIC_LIBS), \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/$(BUILD_TARGET_STATIC_PREFIX)$(lib)$(BUILD_TARGET_STATIC_LIBEXT)) \
  $(eval LOCAL_FINAL_MODULE := $(call local-library-path,$(lib))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE),$(lib), $(LOCAL_SOURCE_DEPENDENCIES))) \
  $(eval $(call install-file,$(LOCAL_BUILT_MODULE),$(LOCAL_FINAL_MODULE))) \
  $(eval LOCAL_SOURCE_DEPENDENCIES += $(LOCAL_BUILT_MODULE)) \
)

$(foreach lib,$(PRODUCED_SHARED_LIBS), \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/lib$(lib)$(call local-build-var,DLLEXT)) \
  $(eval LOCAL_FINAL_MODULE := $(call local-shared-library-path,$(lib))) \
  $(eval LOCAL_INSTALL_MODULE := $(call local-shared-library-install-path,lib$(lib))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE),$(lib), $(LOCAL_SOURCE_DEPENDENCIES))) \
  $(eval $(call install-file,$(LOCAL_BUILT_MODULE),$(LOCAL_FINAL_MODULE))) \
  $(eval $(call install-binary,$(LOCAL_FINAL_MODULE),$(LOCAL_INSTALL_MODULE),--strip-unneeded)) \
  $(eval LOCAL_SOURCE_DEPENDENCIES += $(LOCAL_BUILT_MODULE)) \
)


# Producing an executable consists of the following steps:
# Translate the mapping
# Invoke the right target on make
# install the binary
# generate the symbols
$(foreach prg,$(PRODUCED_EXECUTABLES), \
  $(eval map := $(subst =,$(space),$(prg))) \
  $(eval exe := $(word 1, $(map))) \
  $(eval to  := $(word 2, $(map))) \
  $(eval to  := $(if $(to),$(to),$(exe))) \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/$(exe)) \
  $(eval LOCAL_INSTALL_MODULE := $(call local-executable-install-path,$(to))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT),$(exe), $(LOCAL_SOURCE_DEPENDENCIES))) \
  $(eval $(call install-binary,$(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT),$(LOCAL_INSTALL_MODULE),--strip-all,$(LOCAL_INSTALL_OPENGL))) \
  $(eval LOCAL_SOURCE_DEPENDENCIES += $(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT)) \
  $(if $(and,$(BUILD_GENERATE_SYMBOLS),$(LOCAL_TARGET)), \
    $(eval $(call build-install-debug-info,$(LOCAL_BUILT_MODULE),$(LOCAL_INSTALL_MODULE))) \
    $(eval $(call build-install-symbol,$(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT),$(LOCAL_INSTALL_MODULE))) \
    $(eval _BUILD_SYMBOLS += $(call local-symbol-install-path,$(LOCAL_INSTALL_MODULE))) \
  ) \
)

# The same as above, with the addition that it will add the test to the test runner collection,
# so that it will be invoked by make test.
$(foreach exe,$(PRODUCED_TESTS), \
  $(eval map := $(subst =,$(space),$(exe))) \
  $(eval from := $(word 1, $(map))) \
  $(eval to  := $(word 2, $(map))) \
  $(eval to  := $(if $(to),$(to),$(exe))) \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/$(from)) \
  $(eval LOCAL_INSTALL_MODULE := $(call local-executable-install-path,$(to))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT),$(from), $(LOCAL_SOURCE_DEPENDENCIES))) \
  $(eval $(call install-binary,$(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT),$(LOCAL_INSTALL_MODULE),--strip-all,$(LOCAL_INSTALL_OPENGL))) \
  $(eval $(call run-test,$(LOCAL_INSTALL_MODULE))) \
  $(eval LOCAL_SOURCE_DEPENDENCIES += $(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT)) \
  $(if $(and,$(BUILD_GENERATE_SYMBOLS),$(LOCAL_TARGET)), \
    $(eval $(call build-install-debug-info,$(LOCAL_BUILT_MODULE),$(LOCAL_INSTALL_MODULE))) \
    $(eval $(call build-install-symbol,$(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT),$(LOCAL_INSTALL_MODULE))) \
    $(eval _BUILD_SYMBOLS += $(call local-symbol-install-path,$(LOCAL_INSTALL_MODULE))) \
  ) \
)
