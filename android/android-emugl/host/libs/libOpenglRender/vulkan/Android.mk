LOCAL_PATH := $(call my-dir)

$(call emugl-begin-static-library,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/cereal \
    $(EMUGL_PATH)/host/include/vulkan \

LOCAL_STATIC_LIBRARIES += \
    android-emu \
    android-emu-base \

LOCAL_SRC_FILES := \
    VulkanStream.cpp \

$(call emugl-end-module)

$(call emugl-begin-executable,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan_unittests)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan libemugl_gtest)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan_cereal)

LOCAL_STATIC_LIBRARIES += \
    android-emu \
    android-emu-base \

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/cereal \
    $(EMUGL_PATH)/host/include/vulkan \

LOCAL_SRC_FILES := \
    VulkanStream_unittest.cpp \

$(call emugl-end-module)
