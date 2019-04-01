# Copyright 2018 The Android Open Source Project
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

# The toolchain files get processed multiple times during compile detection
# keep these files simple, and do not setup libraries or anything else.
# merely tags, compiler toolchain and flags should be set here.

get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
include(toolchain)

# First we setup all the tags.
toolchain_configure_tags("darwin-x86_64")

# Make sure we always set the rpath to include current dir and ./lib64. We need this to create self
# contained executables that dynamically load dylibs.
set(RUNTIME_OS_PROPERTIES "INSTALL_RPATH>=@loader_path;INSTALL_RPATH>=@loader_path/lib64;BUILD_WITH_INSTALL_RPATH=ON;INSTALL_RPATH_USE_LINK_PATH=ON")
toolchain_generate("darwin-x86_64")

# Always consider the source to be darwin.
add_definitions(-D_DARWIN_C_SOURCE=1)


# Configure how we strip executables.
set(CMAKE_STRIP_CMD "${CMAKE_STRIP} -S")

# And the asm type if we are compiling with yasm
set(ANDROID_YASM_TYPE macho64)
# No magical includes or dependencies for darwin..
