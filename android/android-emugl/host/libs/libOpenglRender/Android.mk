LOCAL_PATH := $(call my-dir)

$(call emugl-begin-static-library,lib$(BUILD_TARGET_SUFFIX)OSWindow)

LOCAL_C_INCLUDES += \
    $(EMUGL_PATH)/host/libs/libOpenglRender/standalone_common/angle-util \

LOCAL_SRC_FILES += \
    standalone_common/angle-util/OSWindow.cpp \

ifeq ($(BUILD_TARGET_OS),linux)
    LOCAL_SRC_FILES += standalone_common/angle-util/x11/X11Window.cpp
endif

ifeq ($(BUILD_TARGET_OS),darwin)
    LOCAL_SRC_FILES += standalone_common/angle-util/osx/OSXWindow.mm
endif

ifeq ($(BUILD_TARGET_OS_FLAVOR),windows)
    LOCAL_SRC_FILES += standalone_common/angle-util/windows/WindowsTimer.cpp
    LOCAL_SRC_FILES += standalone_common/angle-util/windows/win32/Win32Window.cpp
endif

LOCAL_LDLIBS := -lm

ifeq ($(BUILD_TARGET_OS),linux)
    LOCAL_LDLIBS += -lX11 -lrt
endif

ifeq ($(BUILD_TARGET_OS),darwin)
    LOCAL_LDLIBS += -Wl,-framework,AppKit
endif

ifeq ($(BUILD_TARGET_OS_FLAVOR),windows)
    LOCAL_LDLIBS += -lgdi32
endif

$(call emugl-end-module)

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

ifeq ($(BUILD_TARGET_OS_FLAVOR),windows)
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

# Common configuration for standalone apps such as tests and samples############
standalone_common_SRC_FILES := \
    $(host_common_SRC_FILES) \
    ../Translator/GLES_V2/ANGLEShaderParser.cpp \
    standalone_common/SampleApplication.cpp \
    standalone_common/SearchPathsSetup.cpp \
    standalone_common/ShaderUtils.cpp \


ifeq ($(BUILD_TARGET_OS),linux)
    standalone_common_LDLIBS += -lm -lX11 -lrt
endif

ifeq ($(BUILD_TARGET_OS),darwin)
    standalone_common_LDLIBS += -lm -Wl,-framework,AppKit
endif

ifeq ($(BUILD_TARGET_OS),windows)
    standalone_common_LDLIBS += -lm
endif

ifeq ($(BUILD_TARGET_OS_FLAVOR),windows)
    standalone_common_LDLIBS += -lgdi32
endif

standalone_common_LDFLAGS :=

ifeq ($(BUILD_TARGET_OS),darwin)
    standalone_common_LDFLAGS += -Wl,-headerpad_max_install_names
endif

standalone_common_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/host/include \
    $(EMUGL_PATH)/host/include/vulkan \
    $(EMUGL_PATH)/host/libs/Translator/include \
    $(EMUGL_PATH)/host/libs/Translator/GLES_V2/ \
    $(EMUGL_PATH)/host/libs/libOpenGLESDispatch \
    $(EMUGL_PATH)/host/include/OpenGLESDispatch \
    $(EMUGL_PATH)/host/libs/libGLSnapshot \
    $(ANGLE_TRANSLATION_INCLUDES) \
    $(EMUGL_PATH)/host/libs/libOpenglRender/standalone_common \
    $(EMUGL_PATH)/host/libs/libOpenglRender/standalone_common/angle-util \
    $(EMUGL_PATH)/host/libs/libOpenglRender/vulkan \

standalone_common_STATIC_LIBRARIES := \
    $(ANGLE_TRANSLATION_STATIC_LIBRARIES) \
    libOpenGLESDispatch \
    libGLSnapshot \
    libemugl_common \
    android-emu \
    android-emu-base \
    emulator-lz4 \

### host libOpenglRender #################################################
$(call emugl-begin-shared-library,lib$(BUILD_TARGET_SUFFIX)OpenglRender)

$(call emugl-import,libGLESv1_dec libGLESv2_dec lib_renderControl_dec libOpenglCodecCommon)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan)

LOCAL_LDLIBS += $(host_common_LDLIBS)
LOCAL_LDLIBS += $(ANDROID_EMU_LDLIBS)

LOCAL_SRC_FILES := $(host_common_SRC_FILES)
$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

# use Translator's egl/gles headers
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/include/vulkan
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/libOpenGLESDispatch
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/libOpenglRender/vulkan

LOCAL_STATIC_LIBRARIES += libemugl_common
LOCAL_STATIC_LIBRARIES += libOpenGLESDispatch
LOCAL_STATIC_LIBRARIES += libGLSnapshot

LOCAL_SYMBOL_FILE := render_api.entries

$(call emugl-export,CFLAGS,$(EMUGL_USER_CFLAGS))

$(call emugl-end-module)

# standalone static library for tests and samples###############################
$(call emugl-begin-static-library,lib$(BUILD_TARGET_SUFFIX)OpenglRender_standalone_common)
$(call emugl-import,libOpenglCodecCommon libemugl_gtest)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OSWindow)

LOCAL_C_INCLUDES += $(standalone_common_C_INCLUDES)
LOCAL_STATIC_LIBRARIES += $(standalone_common_STATIC_LIBRARIES)

LOCAL_SRC_FILES := \
    $(standalone_common_SRC_FILES) \

LOCAL_LDFLAGS += $(standalone_common_LDFLAGS)

$(call emugl-end-module)

# Tests#########################################################################
#
# bug: 117790730
# See if skipping this allows mac builds to stop hanging.
# $(call emugl-begin-executable,lib$(BUILD_TARGET_SUFFIX)OpenglRender_unittests)
# $(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender_standalone_common libemugl_gtest)
# $(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OSWindow)
# 
# LOCAL_C_INCLUDES += $(standalone_common_C_INCLUDES)
# LOCAL_STATIC_LIBRARIES += $(standalone_common_STATIC_LIBRARIES)
# 
# LOCAL_SRC_FILES := \
#     samples/HelloTriangleImp.cpp \
#     tests/DefaultFramebufferBlit_unittest.cpp \
#     tests/FrameBuffer_unittest.cpp \
#     tests/GLSnapshot_unittest.cpp \
#     tests/GLSnapshotBuffers_unittest.cpp \
#     tests/GLSnapshotFramebufferControl_unittest.cpp \
#     tests/GLSnapshotFramebuffers_unittest.cpp \
#     tests/GLSnapshotMultisampling_unittest.cpp \
#     tests/GLSnapshotPixelOperations_unittest.cpp \
#     tests/GLSnapshotPixels_unittest.cpp \
#     tests/GLSnapshotPrograms_unittest.cpp \
#     tests/GLSnapshotRasterization_unittest.cpp \
#     tests/GLSnapshotRenderbuffers_unittest.cpp \
#     tests/GLSnapshotRendering_unittest.cpp \
#     tests/GLSnapshotShaders_unittest.cpp \
#     tests/GLSnapshotTestDispatch.cpp \
#     tests/GLSnapshotTesting.cpp \
#     tests/GLSnapshotTestStateUtils.cpp \
#     tests/GLSnapshotTextures_unittest.cpp \
#     tests/GLSnapshotTransformation_unittest.cpp \
#     tests/GLSnapshotVertexAttributes_unittest.cpp \
#     tests/GLTestUtils.cpp \
#     tests/OpenGL_unittest.cpp \
#     tests/OpenGLTestContext.cpp \
#     tests/StalePtrRegistry_unittest.cpp \
#     tests/TextureDraw_unittest.cpp \
#     tests/Vulkan_unittest.cpp \
# 
# LOCAL_LDFLAGS += $(standalone_common_LDFLAGS)
# LOCAL_LDLIBS += $(standalone_common_LDLIBS)
# 
# LOCAL_INSTALL_OPENGL := true
# ifeq ($(BUILD_TARGET_OS),linux)
# LOCAL_LDFLAGS += '-Wl,-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX),-rpath,$$ORIGIN/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader'
# endif
# $(call emugl-end-module)

# Samples#######################################################################

make_sample = \
    $(eval $(call emugl-begin-executable, $1)) \
    $(eval $(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender_standalone_common)) \
    $(eval $(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OSWindow)) \
    $(eval LOCAL_C_INCLUDES += $(standalone_common_C_INCLUDES)) \
    $(eval LOCAL_STATIC_LIBRARIES += $(standalone_common_STATIC_LIBRARIES)) \
    $(eval LOCAL_SRC_FILES := samples/$1.cpp) \
    $(eval LOCAL_SRC_FILES += $(if $(wildcard $(EMUGL_PATH)/host/libs/libOpenglRender/samples/$1Imp.cpp), samples/$1Imp.cpp,)) \
    $(eval LOCAL_LDFLAGS += $(standalone_common_LDFLAGS)) \
    $(eval LOCAL_INSTALL_OPENGL := true) \
    $(eval LOCAL_LDFLAGS += $(standalone_common_LDFLAGS)) \
    $(eval LOCAL_LDLIBS += $(standalone_common_LDLIBS)) \
    $(if $(filter linux,$(BUILD_TARGET_OS)), \
      $(eval LOCAL_LDFLAGS += -Wl,-rpath,\$$$$ORIGIN/lib$(BUILD_TARGET_SUFFIX),-rpath,\$$$$ORIGIN/lib$(BUILD_TARGET_SUFFIX)/gles_swiftshader)) \
    $(eval $(call emugl-end-module)) \

# Only build samples on 64-bit hosts
ifeq ($(BUILD_TARGET_SUFFIX),64)

$(call make_sample,HelloTriangle)
$(call make_sample,HelloSurfaceFlinger)
$(call make_sample,CreateDestroyContext)
$(call make_sample,HelloHostComposition)

endif
