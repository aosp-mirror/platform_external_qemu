LOCAL_PATH := $(call my-dir)

combined_SHARED_LIBRARIES := \
    android-emu-shared \
    libcutils \
    libgui \
    libOpenglSystemCommon \
    libEGL_emulation \
    libGLESv2_emulation \

combined_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/guest \
    $(EMUGL_PATH)/guest/androidImpl \
    $(EMUGL_PATH)/host/include \
    $(EMUGL_PATH)/host/libs/libOpenglRender/standalone_common/angle-util \
    $(ANDROID_EMU_BASE_INCLUDES) \

combined_SRC_FILES := \
    ClientComposer.cpp \
    Display.cpp \
    GoldfishOpenglTestEnv.cpp \
    Toplevel.cpp \

# Shared library that combines guest driver with host drivers###################
$(call emugl-begin-shared-library,libaemugraphics)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OSWindow)
LOCAL_CFLAGS += -fvisibility=default
LOCAL_C_INCLUDES += $(combined_C_INCLUDES)
LOCAL_SRC_FILES += $(combined_SRC_FILES)
$(call emugl-export,SHARED_LIBRARIES, $(combined_SHARED_LIBRARIES))
$(call emugl-end-module)

# Unit tests for individual components of the combined library##################
$(call emugl-begin-executable,emugl_combined_unittests)
$(call emugl-import,libaemugraphics libemugl_gtest)

LOCAL_C_INCLUDES += $(combined_C_INCLUDES)
LOCAL_SRC_FILES += \
    GoldfishOpenglTestEnv.cpp \
    combined_unittest.cpp \

ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_LDFLAGS += '-Wl,-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX),-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader'
LOCAL_LDLIBS += -ldl
endif
LOCAL_INSTALL_OPENGL := true
$(call emugl-end-module)

# Unit tests for the Toplevel class#############################################
$(call emugl-begin-executable,libaemugraphics_toplevel_unittests)
$(call emugl-import,libaemugraphics libemugl_gtest)

LOCAL_C_INCLUDES += $(combined_C_INCLUDES)
LOCAL_SRC_FILES +=  \
    toplevel_unittest.cpp \

ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_LDFLAGS += '-Wl,-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX),-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader'
LOCAL_LDLIBS += -ldl
endif
LOCAL_INSTALL_OPENGL := true
$(call emugl-end-module)