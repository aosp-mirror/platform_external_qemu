LOCAL_PATH := $(call my-dir)

$(call emugl-begin-executable,emugl_combined_unittests)

$(call emugl-import,libemugl_gtest)

$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/guest \
    $(EMUGL_PATH)/guest/androidImpl \
    $(EMUGL_PATH)/host/include \
    $(ANDROID_EMU_BASE_INCLUDES) \

ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_LDFLAGS += '-Wl,-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX),-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader'
endif

LOCAL_SRC_FILES += combined_unittest.cpp

LOCAL_SHARED_LIBRARIES += \
    android-emu-shared \
    libOpenglSystemCommon \
    libEGL_emulation \
    libcutils \
    libGLESv2_emulation \

LOCAL_INSTALL_OPENGL := true

$(call emugl-end-module)