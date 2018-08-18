
LOCAL_PATH := $(call my-dir)

# For Vulkan libraries

cereal_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/host/include/vulkan \

cereal_STATIC_LIBRARIES := \
    android-emu \
    android-emu-base \


$(call emugl-begin-static-library,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan_cereal_guest)

LOCAL_C_INCLUDES += $(cereal_C_INCLUDES)

LOCAL_STATIC_LIBRARIES += $(cereal_STATIC_LIBRARIES)

LOCAL_SRC_FILES := \
    guest/goldfish_vk_frontend.cpp \

$(call emugl-end-module)

