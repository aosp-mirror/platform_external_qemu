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

# first, call a library containing all object files
LOCAL_BUILT_MODULE := $(call local-shared-library-path,$(LOCAL_MODULE))
include $(_BUILD_CORE_DIR)/emulator/binary.make

LOCAL_LIBRARIES := $(foreach lib,\
    $(LOCAL_WHOLE_STATIC_LIBRARIES) $(LOCAL_STATIC_LIBRARIES),\
    $(call local-library-path,$(lib)))

ifeq ($(BUILD_TARGET_OS),darwin)
    LOCAL_SHARED_LIBRARY_LDLIBS := \
        $(foreach lib,$(LOCAL_SHARED_LIBRARIES),$(call local-shared-library-path,$(lib)))
else
    LOCAL_LDFLAGS += \
        $(foreach lib,$(LOCAL_SHARED_LIBRARIES), \
            -L$(dir $(call local-shared-library-path,$(lib)))) \

    ifeq ($(BUILD_TARGET_OS),windows_msvc)
        # clang will generate an import lib for any generated dll, so link to that .lib file.
        LOCAL_SHARED_LIBRARY_LDLIBS := \
            $(foreach lib,$(LOCAL_SHARED_LIBRARIES), $(subst .dll,, -l$(notdir $(call local-shared-library-path,$(lib)))))
    else
        LOCAL_SHARED_LIBRARY_LDLIBS := \
            $(foreach lib,$(LOCAL_SHARED_LIBRARIES), -l:$(notdir $(call local-shared-library-path,$(lib))))
    endif
endif

LOCAL_LDLIBS := $(call local-static-libraries-ldlibs) $(LOCAL_SHARED_LIBRARY_LDLIBS) $(LOCAL_LDLIBS)

$(LOCAL_BUILT_MODULE): PRIVATE_LDFLAGS := $(LDFLAGS) $(LOCAL_LDFLAGS)
$(LOCAL_BUILT_MODULE): PRIVATE_LDLIBS  := $(LOCAL_LDLIBS)
$(LOCAL_BUILT_MODULE): PRIVATE_LD      := $(LOCAL_LD)
$(LOCAL_BUILT_MODULE): PRIVATE_OBJS    := $(LOCAL_OBJECTS)

LOCAL_SYMBOL_FILE := $(strip $(LOCAL_SYMBOL_FILE))
ifdef LOCAL_SYMBOL_FILE
$(eval $(call generate-symbol-file,$(LOCAL_PATH)/$(LOCAL_SYMBOL_FILE)))
$(LOCAL_BUILT_MODULE): $(LOCAL_GENERATED_SYMBOL_FILE)
$(LOCAL_BUILT_MODULE): PRIVATE_LDFLAGS += $(call symbol-file-linker-flags,$(LOCAL_GENERATED_SYMBOL_FILE))
endif

$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS) $(LOCAL_LIBRARIES)
	@ mkdir -p $(dir $@)
	@ echo "SharedLibrary: $@"
	$(hide) $(PRIVATE_LD) $(PRIVATE_LDFLAGS) -shared -o $@ $(PRIVATE_LIBRARY) $(PRIVATE_OBJS) $(PRIVATE_LDLIBS)

_BUILD_EXECUTABLES += $(LOCAL_BUILT_MODULE)
$(LOCAL_BUILT_MODULE): $(foreach lib,$(LOCAL_STATIC_LIBRARIES),$(call local-library-path,$(lib)))
$(LOCAL_BUILT_MODULE): $(foreach lib,$(LOCAL_SHARED_LIBRARIES),$(call local-shared-library-path,$(lib)))

ifeq (true,$(LOCAL_INSTALL))
LOCAL_INSTALL_MODULE := $(call local-shared-library-install-path,$(LOCAL_MODULE))
$(eval $(call install-binary,$(LOCAL_BUILT_MODULE),$(LOCAL_INSTALL_MODULE),--strip-unneeded))

include $(_BUILD_CORE_DIR)/emulator/symbols.make
endif  # LOCAL_INSTALL == true
