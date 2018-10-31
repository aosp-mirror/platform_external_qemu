# Copyright 2015 The Android Open Source Project
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

# DO NOT MODIFY THIS FILE

# Check that we have at least GNU Make 3.81
# We do this by detecting whether 'lastword' is supported
#
MAKE_TEST := $(lastword a b c d e f)
ifneq ($(MAKE_TEST),f)
    $(error,This build system requires GNU Make 3.81 or higher to run !)
endif

# Find the source installation path, should be this file's location.
_BUILD_ROOT := $(dir $(lastword $(MAKEFILE_LIST)))
_BUILD_ROOT := $(_BUILD_ROOT:%/=%)

# Complain if the path contains spaces
ifneq ($(words $(_BUILD_ROOT)),1)
    $(info,The source installation path contains spaces: '$(_BUILD_ROOT)')
    $(error,Please fix the problem by reinstalling to a different location.)
endif

message:
		@echo "The GNUMake system has been deprecated, please use cmake." 
		@echo "source ./android/envsetup.sh"
		@echo "./android/rebuild.sh"
# END OF FILE

