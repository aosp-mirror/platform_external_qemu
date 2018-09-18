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

include $(_BUILD_CORE_DIR)/emulator/binary.make

# Generate the proper cmake translation target, based upon the host build property
ifeq ($(strip $(LOCAL_HOST_BUILD)),)
 $(eval $(call cmake-project-target,$(LOCAL_CMAKELISTS),$(LOCAL_BUILT_MAKEFILE)))
 $(eval LOCAL_EXEEXT:=$(BUILD_TARGET_EXEEXT))
else
 $(eval $(call cmake-project-host,$(LOCAL_CMAKELISTS),$(LOCAL_BUILT_MAKEFILE)))
 $(eval LOCAL_EXEEXT:=$(BUILD_HOST_EXEEXT))
endif

$(foreach prg,$(PRODUCED_EXECUTABLES), \
  $(eval map := $(subst =,$(space),$(prg))) \
  $(eval exe := $(word 1, $(map))) \
  $(eval to  := $(word 2, $(map))) \
  $(eval to  := $(if $(to),$(to),$(exe))) \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/$(exe)) \
  $(eval LOCAL_INSTALL_MODULE := $(call local-executable-install-path,$(exe))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE),$(exe))) \
  $(eval $(call install-binary,$(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT),$(LOCAL_INSTALL_MODULE),--strip-all,$(LOCAL_INSTALL_OPENGL))) \
)

$(foreach exe,$(PRODUCED_TESTS), \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/$(exe)) \
  $(eval LOCAL_INSTALL_MODULE := $(call local-executable-install-path,$(exe))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE),$(exe))) \
  $(eval $(call install-binary,$(LOCAL_BUILT_MODULE)$(LOCAL_EXEEXT),$(LOCAL_INSTALL_MODULE),--strip-all,$(LOCAL_INSTALL_OPENGL))) \
  $(eval $(call run-test,$(LOCAL_INSTALL_MODULE))) \
)

$(foreach lib,$(PRODUCED_STATIC_LIBS), \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/lib$(lib).a) \
  $(eval LOCAL_FINAL_MODULE := $(call local-library-path,$(lib))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE),$(lib))) \
  $(eval $(call install-file,$(LOCAL_BUILT_MODULE),$(LOCAL_FINAL_MODULE))) \
)

