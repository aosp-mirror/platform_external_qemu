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

$(call emugl-begin-static-library,libgrallocusage)

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SRC_FILES += libgrallocusage/GrallocUsageConversion.cpp

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