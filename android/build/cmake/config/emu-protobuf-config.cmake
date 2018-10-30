# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.

# Note, different versions of cmake handle protobuf resolution differently!

get_filename_component(PREBUILT_ROOT
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/protobuf/${ANDROID_TARGET_TAG}"
                       ABSOLUTE)

set(PROTOBUF_INCLUDE_DIR "${PREBUILT_ROOT}/include")
set(PROTOBUF_INCLUDE_DIRS "${PROTOBUF_INCLUDE_DIR}")
set(PROTOBUF_LIBRARIES "${PREBUILT_ROOT}/lib/libprotobuf${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(PROTOBUF_LIBRARY "${PREBUILT_ROOT}/lib/libprotobuf${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(PROTOBUF_IMPORT_DIRS "${PROTOBUF_INCLUDE_DIR}")
set(PROTOBUF_FOUND TRUE)

# So cmake is not clear on what type of casing to use in the various Cmake versions. We should be using our official
# shipped in cmake binary (3.5), but maybe you are not using that one, so let's work around it for now.
#
# See: https://cmake.org/cmake/help/v3.5/module/FindProtobuf.html vs
# https://cmake.org/cmake/help/v3.7/module/FindProtobuf.html
set(Protobuf_FOUND TRUE)
set(Protobuf_INCLUDE_DIRS "${PROTOBUF_INCLUDE_DIRS}")
set(Protobuf_LIBRARIES "${PROTOBUF_LIBRARIES}")
set(Protobuf_LIBRARY "${PROTOBUF_LIBRARY}")
set(Protobuf_IMPORT_DIRS "${PROTOBUF_IMPORT_DIRS}")

if(ANDROID_TARGET_TAG MATCHES ".*windows.*")
  # We better be able to run the protoc tool when we are cross compiling...
  get_filename_component(PROTO_EXEC_ROOT
                         "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/protobuf/linux-x86_64"
                         ABSOLUTE)
  set(PROTOBUF_PROTOC_EXECUTABLE "${PROTO_EXEC_ROOT}/bin/protoc")
  set(Protobuf_PROTOC_EXECUTABLE "${PROTOBUF_PROTOC_EXECUTABLE}")
  message(STATUS "Cross compiling using linux protoc")
else()
  set(PROTOBUF_PROTOC_EXECUTABLE "${PREBUILT_ROOT}/bin/protoc")
  set(Protobuf_PROTOC_EXECUTABLE "${PROTOBUF_PROTOC_EXECUTABLE}")
endif()

set(
  PACKAGE_EXPORT
  "PROTOBUF_INCLUDE_DIRS;PROTOBUF_IMPORT_DIRS;PROTOBUF_INCLUDE_DIR;PROTOBUF_LIBRARIES;PROTOBUF_PROTOC_EXECUTABLE;PROTOBUF_FOUND"
  )

if (NOT TARGET protobuf::libprotobuf)
  add_library(protobuf::libprotobuf INTERFACE IMPORTED GLOBAL)
  set_target_properties(protobuf::libprotobuf PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${PROTOBUF_LIBRARIES}"
  )
endif()


# We've configured everything, so now let's make sure cmake sets up the right macros
find_package(Protobuf REQUIRED)



# Prevent find_package from overring our lib
set(PROTOBUF_LIBRARIES "${PREBUILT_ROOT}/lib/libprotobuf${CMAKE_STATIC_LIBRARY_SUFFIX}")
