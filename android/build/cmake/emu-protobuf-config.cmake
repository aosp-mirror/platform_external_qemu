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

get_filename_component(PREBUILT_ROOT "${LOCAL_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/protobuf/${LOCAL_TARGET_TAG}" ABSOLUTE)

set(PROTOBUF_INCLUDE_DIRS "${PREBUILT_ROOT}/include")
set(PROTOBUF_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(PROTOBUF_LIBRARIES "${PREBUILT_ROOT}/lib/libprotobuf.a")
set(PROTOBUF_LIBRARY "${PREBUILT_ROOT}/lib/libprotobuf.a")
set(PROTOBUF_PROTOC_EXECUTABLE "${PREBUILT_ROOT}/bin/protoc")
set(PROTOBUF_IMPORT_DIRS "${PROTOBUF_INCLUDE_DIR}/include")
set(PROTOBUF_FOUND TRUE)



set(PACKAGE_EXPORT "PROTOBUF_INCLUDE_DIRS;PROTOBUF_IMPORT_DIRS;PROTOBUF_INCLUDE_DIR;PROTOBUF_LIBRARIES;PROTOBUF_PROTOC_EXECUTABLE;PROTOBUF_FOUND")

# We've configured everything, so now let's make sure cmake sets up the right macros
find_package(Protobuf REQUIRED)

