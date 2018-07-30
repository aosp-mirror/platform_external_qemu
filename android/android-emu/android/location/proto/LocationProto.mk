#  Copyright (C) 2018 The Android Open Source Project
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

#
# A static library containing the Location protobuf generated code
#

# Compute LOCATIONPROTO_DIR relative to LOCAL_PATH

LOCATIONPROTO_DIR := $(call my-dir)
LOCATIONPROTO_DIR := $(LOCATIONPROTO_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,liblocation_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(LOCATIONPROTO_DIR)/point.proto \
    $(LOCATIONPROTO_DIR)/route.proto \

$(call end-emulator-library)

LOCATION_PROTO_STATIC_LIBRARIES := \
    liblocation_proto \
    $(PROTOBUF_STATIC_LIBRARIES)
