LOCAL_PATH := $(call my-dir)

$(call emugl-begin-static-library,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan_cereal)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/cereal \
    $(EMUGL_PATH)/host/include/vulkan \

LOCAL_STATIC_LIBRARIES += \
    android-emu \
    android-emu-base \

LOCAL_SRC_FILES := \
    VulkanDispatch.cpp \
    VulkanStream.cpp \

ifeq (windows_msvc, $(BUILD_TARGET_OS))
    LOCAL_C_INCLUDES += $(MSVC_POSIX_COMPAT_INCLUDES)
    LOCAL_STATIC_LIBRARIES += $(MSVC_POSIX_COMPAT_INCLUDES)
endif

$(call emugl-end-module)

$(call emugl-begin-executable,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan_unittests)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan libemugl_gtest)

LOCAL_STATIC_LIBRARIES += \
    android-emu \
    android-emu-base \

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/cereal \
    $(EMUGL_PATH)/host/include/vulkan \

LOCAL_SRC_FILES := \
    VulkanStream_unittest.cpp \

$(call emugl-end-module)
