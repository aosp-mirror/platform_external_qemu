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

# first we generate the cmake project
LOCAL_CMAKE_MODULE   :=$(call local-cmake-path,$(LOCAL_MODULE))
LOCAL_CMAKELISTS     :=$(LOCAL_PATH)/CMakeLists.txt
LOCAL_BUILT_MAKEFILE :=$(LOCAL_CMAKE_MODULE)/Makefile


include $(_BUILD_CORE_DIR)/emulator/binary.make
$(eval $(call cmake-project-host,$(LOCAL_CMAKELISTS),$(LOCAL_BUILT_MAKEFILE)))

$(foreach exe,$(PRODUCED_EXECUTABLES), \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/$(exe)) \
  $(eval LOCAL_INSTALL_MODULE := $(call local-executable-install-path,$(exe))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE),$(exe))) \
  $(eval $(call install-binary,$(LOCAL_BUILT_MODULE),$(LOCAL_INSTALL_MODULE),--strip-all,$(LOCAL_INSTALL_OPENGL))) \
)

$(foreach exe,$(PRODUCED_TESTS), \
  $(eval LOCAL_BUILT_MODULE := $(LOCAL_CMAKE_MODULE)/$(exe)) \
  $(eval LOCAL_INSTALL_MODULE := $(call local-executable-install-path,$(exe))) \
  $(eval $(call make-cmake-project,$(LOCAL_BUILT_MAKEFILE),$(LOCAL_BUILT_MODULE),$(exe))) \
  $(eval $(call install-binary,$(LOCAL_BUILT_MODULE),$(LOCAL_INSTALL_MODULE),--strip-all,$(LOCAL_INSTALL_OPENGL))) \
  $(eval $(call run-test,$(LOCAL_INSTALL_MODULE))) \
)

