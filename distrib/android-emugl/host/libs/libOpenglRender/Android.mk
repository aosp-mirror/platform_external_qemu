LOCAL_PATH := $(call my-dir)

host_OS_SRCS :=
host_common_LDLIBS :=

ifeq ($(HOST_OS),linux)
    host_OS_SRCS = NativeSubWindow_x11.cpp
    host_common_LDLIBS += -lX11 -lrt
endif

ifeq ($(HOST_OS),darwin)
    host_OS_SRCS = NativeSubWindow_cocoa.m
    host_common_LDLIBS += -Wl,-framework,AppKit
endif

ifeq ($(HOST_OS),windows)
    host_OS_SRCS = NativeSubWindow_win32.cpp
endif

host_common_SRC_FILES := \
    $(host_OS_SRCS) \
    ColorBuffer.cpp \
    EGLDispatch.cpp \
    FbConfig.cpp \
    FrameBuffer.cpp \
    GLESv1Dispatch.cpp \
    GLESv2Dispatch.cpp \
    ReadBuffer.cpp \
    RenderContext.cpp \
    RenderControl.cpp \
    RenderServer.cpp \
    RenderThread.cpp \
    RenderThreadInfo.cpp \
    render_api.cpp \
    RenderWindow.cpp \
    TextureDraw.cpp \
    WindowSurface.cpp \

host_common_CFLAGS :=

#For gl debbuging
#host_common_CFLAGS += -DCHECK_GL_ERROR


### host libOpenglRender #################################################
$(call emugl-begin-host-shared-library,libOpenglRender)

$(call emugl-import,libGLESv1_dec libGLESv2_dec lib_renderControl_dec libOpenglCodecCommon)

LOCAL_LDLIBS += $(host_common_LDLIBS)

LOCAL_SRC_FILES := $(host_common_SRC_FILES)
$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

# use Translator's egl/gles headers
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include

LOCAL_STATIC_LIBRARIES += libemugl_common

LOCAL_SYMBOL_FILE := render_api.entries

$(call emugl-export,CFLAGS,$(host_common_CFLAGS))

$(call emugl-end-module)


### host libOpenglRender, 64-bit #########################################
$(call emugl-begin-host64-shared-library,lib64OpenglRender)

$(call emugl-import,lib64GLESv1_dec lib64GLESv2_dec lib64_renderControl_dec lib64OpenglCodecCommon)

LOCAL_LDLIBS += $(host_common_LDLIBS)

LOCAL_SRC_FILES := $(host_common_SRC_FILES)
$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

# use Translator's egl/gles headers
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include

LOCAL_STATIC_LIBRARIES += lib64emugl_common

$(call emugl-export,CFLAGS,$(host_common_CFLAGS))

$(call emugl-end-module)
