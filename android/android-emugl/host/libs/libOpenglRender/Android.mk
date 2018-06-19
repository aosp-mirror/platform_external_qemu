LOCAL_PATH := $(call my-dir)

host_OS_SRCS :=
host_common_LDLIBS :=

ifeq ($(BUILD_TARGET_OS),linux)
    host_OS_SRCS = NativeSubWindow_x11.cpp
    host_common_LDLIBS += -lX11 -lrt
endif

ifeq ($(BUILD_TARGET_OS),darwin)
    host_OS_SRCS = NativeSubWindow_cocoa.m
    host_common_LDLIBS += -Wl,-framework,AppKit
endif

ifeq ($(BUILD_TARGET_OS),windows)
    host_OS_SRCS = NativeSubWindow_win32.cpp
    host_common_LDLIBS += -lgdi32
endif

host_common_SRC_FILES := \
    $(host_OS_SRCS) \
    ChannelStream.cpp \
    ColorBuffer.cpp \
    FbConfig.cpp \
    FenceSync.cpp \
    FrameBuffer.cpp \
    GLESVersionDetector.cpp \
    PostWorker.cpp \
    ReadbackWorker.cpp \
    ReadBuffer.cpp \
    RenderChannelImpl.cpp \
    RenderContext.cpp \
    RenderControl.cpp \
    RendererImpl.cpp \
    RenderLibImpl.cpp \
    RenderThread.cpp \
    RenderThreadInfo.cpp \
    render_api.cpp \
    RenderWindow.cpp \
    SyncThread.cpp \
    TextureDraw.cpp \
    TextureResize.cpp \
    WindowSurface.cpp \
    YUVConverter.cpp \

### host libOpenglRender #################################################
$(call emugl-begin-shared-library,lib$(BUILD_TARGET_SUFFIX)OpenglRender)

$(call emugl-import,libGLESv1_dec libGLESv2_dec lib_renderControl_dec libOpenglCodecCommon)

LOCAL_LDLIBS += $(host_common_LDLIBS)
LOCAL_LDLIBS += $(ANDROID_EMU_LDLIBS)

LOCAL_SRC_FILES := $(host_common_SRC_FILES)
$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

# use Translator's egl/gles headers
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/libOpenGLESDispatch

LOCAL_STATIC_LIBRARIES += libemugl_common
LOCAL_STATIC_LIBRARIES += libOpenGLESDispatch
LOCAL_STATIC_LIBRARIES += libGLSnapshot

LOCAL_SYMBOL_FILE := render_api.entries

$(call emugl-export,CFLAGS,$(EMUGL_USER_CFLAGS))

$(call emugl-end-module)

### OpenglRender unittests
$(call emugl-begin-executable,lib$(BUILD_TARGET_SUFFIX)OpenglRender_unittests)
$(call emugl-import,libOpenglCodecCommon)

$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
$(call emugl-export,LDLIBS,-lm)

ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_LDFLAGS += -Wl,-rpath,$(BUILD_OBJS_DIR)/lib$(BUILD_TARGET_SUFFIX),-rpath,$(BUILD_OBJS_DIR)/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader
endif

# use Translator's egl/gles headers
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/GLES_V2/
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/libOpenGLESDispatch
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/include/OpenGLESDispatch
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/libGLSnapshot
LOCAL_C_INCLUDES += $(ANGLE_TRANSLATION_INCLUDES)
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/libOpenglRender/tests/angle-util
LOCAL_STATIC_LIBRARIES += $(ANGLE_TRANSLATION_STATIC_LIBRARIES)
LOCAL_STATIC_LIBRARIES += libOpenGLESDispatch
LOCAL_STATIC_LIBRARIES += libGLSnapshot
LOCAL_STATIC_LIBRARIES += libemugl_common
LOCAL_STATIC_LIBRARIES += android-emu
LOCAL_STATIC_LIBRARIES += android-emu-base
LOCAL_STATIC_LIBRARIES += emulator-lz4

LOCAL_SYMBOL_FILE := render_api.entries

LOCAL_INSTALL_OPENGL := true

LOCAL_SRC_FILES := \
    $(host_common_SRC_FILES) \
    ../Translator/GLES_V2/ANGLEShaderParser.cpp \
    tests/angle-util/OSWindow.cpp \
    tests/FrameBuffer_unittest.cpp \
    tests/GLSnapshot_unittest.cpp \
    tests/GLSnapshotTesting.cpp \
    tests/GLSnapshotTransformation_unittest.cpp \
    tests/GLSnapshotRasterization_unittest.cpp \
    tests/GLSnapshotMultisampling_unittest.cpp \
    tests/GLSnapshotPixelOperations_unittest.cpp \
    tests/GLSnapshotShaders_unittest.cpp \
    tests/GLTestGlobals.cpp \
    tests/GLTestUtils.cpp \
    tests/OpenGL_unittest.cpp \
    tests/OpenGLTestContext.cpp \
    tests/StalePtrRegistry_unittest.cpp \
    tests/TextureDraw_unittest.cpp \

ifeq ($(BUILD_TARGET_OS),linux)
    LOCAL_SRC_FILES += tests/angle-util/x11/X11Window.cpp
    LOCAL_LDLIBS += -lX11 -lrt
endif

ifeq ($(BUILD_TARGET_OS),darwin)
    LOCAL_SRC_FILES += tests/angle-util/osx/OSXWindow.mm
    LOCAL_LDLIBS += -Wl,-framework,AppKit
	LOCAL_LDFLAGS += -Wl,-headerpad_max_install_names
endif

ifeq ($(BUILD_TARGET_OS),windows)
    LOCAL_SRC_FILES += tests/angle-util/windows/WindowsTimer.cpp
    LOCAL_SRC_FILES += tests/angle-util/windows/win32/Win32Window.cpp
    LOCAL_LDLIBS += -lgdi32
endif

$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender libemugl_gtest)
$(call emugl-end-module)
