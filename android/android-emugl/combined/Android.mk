LOCAL_PATH := $(call my-dir)

$(call emugl-begin-executable,emugl_combined_unittests)

$(call emugl-import,libemugl_gtest)

$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/host/include \
    $(ANDROID_EMU_BASE_INCLUDES) \

LOCAL_SRC_FILES += combined_unittest.cpp

LOCAL_SHARED_LIBRARIES += \
    android-emu-shared \
    libOpenglSystemCommon \
    libEGL_emulation \

$(call emugl-end-module)