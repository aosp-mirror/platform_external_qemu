LOCAL_PATH := $(call my-dir)

combined_SHARED_LIBRARIES = \
    android-emu-shared \
    libcutils \
    libgui \
    libOpenglSystemCommon \
    libEGL_emulation \
    libGLESv2_emulation \

$(call emugl-begin-shared-library,libaemugraphics)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/guest \
    $(EMUGL_PATH)/guest/androidImpl \
    $(EMUGL_PATH)/host/include \
    $(ANDROID_EMU_BASE_INCLUDES) \

LOCAL_SRC_FILES += \
    ClientComposer.cpp \

$(call emugl-export,SHARED_LIBRARIES, $(combined_SHARED_LIBRARIES))

$(call emugl-end-module)

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

LOCAL_SRC_FILES += \
    ClientComposer.cpp \
    combined_unittest.cpp \

ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_LDFLAGS += '-Wl,-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX),-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader'
LOCAL_LDLIBS += -ldl
endif

LOCAL_INSTALL_OPENGL := true

$(call emugl-export,SHARED_LIBRARIES, $(combined_SHARED_LIBRARIES))
$(call emugl-end-module)