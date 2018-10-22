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
LOCAL_PATH := $(call my-dir)

# Implementation of enough Android for graphics#################################

# Android libraries
$(call emugl-begin-shared-library,libutils)
LOCAL_SRC_FILES := \
    androidImpl/libutils_placeholder.cpp \

$(call emugl-end-module)

$(call emugl-begin-shared-library,libcutils)
LOCAL_SRC_FILES := \
    androidImpl/Ashmem.cpp \
    androidImpl/Properties.cpp \
    androidImpl/GrallocDispatch.cpp \

$(call emugl-end-module)

$(call emugl-begin-shared-library,liblog)
LOCAL_C_INCLUDES := $(LOCAL_PATH) $(ANDROID_EMU_BASE_INCLUDES)
LOCAL_SRC_FILES := \
    androidImpl/Log.cpp \

$(call emugl-end-module)

$(call emugl-begin-shared-library,libgui)

LOCAL_CFLAGS += -fvisibility=default

LOCAL_SRC_FILES := \
    androidImpl/AndroidBufferQueue.cpp \
    androidImpl/ANativeWindow.cpp \
    androidImpl/AndroidWindow.cpp \
    androidImpl/AndroidWindowBuffer.cpp \
    androidImpl/SurfaceFlinger.cpp \
    androidImpl/Vsync.cpp \
    sync/sync.cpp \

$(call emugl-end-module)

# Android Vulkan loader/libvulkan
$(call emugl-begin-static-library,libgrallocusage)

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SRC_FILES += libgrallocusage/GrallocUsageConversion.cpp

$(call emugl-end-module)

$(call emugl-begin-shared-library,libvulkan_android)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/vulkan \
    ../host/include \

LOCAL_CFLAGS += \
    -DLOG_TAG=\"vulkan\" \
    -DVK_USE_PLATFORM_ANDROID_KHR \
    -DVK_NO_PROTOTYPES \
    -fvisibility=hidden \
    -fstrict-aliasing \

LOCAL_SRC_FILES := \
    libvulkan/api.cpp \
    libvulkan/api_gen.cpp \
    libvulkan/debug_report.cpp \
    libvulkan/driver.cpp \
    libvulkan/driver_gen.cpp \
    libvulkan/layers_extensions.cpp \
    libvulkan/stubhal.cpp \
    libvulkan/swapchain.cpp \

LOCAL_STATIC_LIBRARIES += libgrallocusage \

$(call emugl-export,SHARED_LIBRARIES, liblog libgui libcutils libutils)

$(call emugl-end-module)

# Unit tests for Android graphics###############################################

$(call emugl-begin-executable,libgui_unittests)
$(call emugl-import,libemugl_gtest)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/guest \
    $(EMUGL_PATH)/guest/androidImpl \
    $(EMUGL_PATH)/host/include \
    $(ANDROID_EMU_BASE_INCLUDES) \

LOCAL_SRC_FILES += \
    androidImpl/libgui_unittest.cpp \

LOCAL_SHARED_LIBRARIES += \
    libgui \

ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_LDFLAGS += '-Wl,-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX),-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader'
endif
LOCAL_INSTALL_OPENGL := true

$(call emugl-end-module)

$(call emugl-begin-executable,vulkanhal_unittests)
$(call emugl-import,libvulkan_android libemugl_gtest)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/guest \
    $(EMUGL_PATH)/guest/androidImpl \
    $(EMUGL_PATH)/host/include \
    $(ANDROID_EMU_BASE_INCLUDES) \

LOCAL_SRC_FILES += \
    androidImpl/vulkanhal_unittest.cpp \

ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_LDFLAGS += '-Wl,-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX),-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader'
LOCAL_LDLIBS += -ldl
endif

LOCAL_INSTALL_OPENGL := true

$(call emugl-end-module)
