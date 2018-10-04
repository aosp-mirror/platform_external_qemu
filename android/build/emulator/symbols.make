# Copyright (C) 2015 The Android Open Source Project
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
ifeq (true,$(BUILD_GENERATE_SYMBOLS))
$(eval $(call build-install-debug-info,$(LOCAL_BUILT_MODULE),$(LOCAL_INSTALL_MODULE)))
$(eval $(call build-install-symbol,$(LOCAL_BUILT_MODULE),$(LOCAL_INSTALL_MODULE)))
_BUILD_SYMBOLS += $(call local-symbol-install-path,$(LOCAL_INSTALL_MODULE))
endif
