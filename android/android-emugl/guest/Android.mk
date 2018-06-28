LOCAL_PATH := $(call my-dir)

goldfish_opengl_DIR := $(ANDROID_EMU_BASE_INCLUDES) goldfish-opengl

goldfish_opengl_CFLAGS := -fvisibility=internal -DANDROID -DEMULATOR_OPENGL_POST_O=1 -DPLATFORM_SDK_VERSION=28 -DHOST_BUILD

goldfish_opengl_INCLUDES := \
	$(LOCAL_PATH) \
    $(EMUGL_PATH)/guest/goldfish-opengl/host/include/libOpenglRender \
    $(EMUGL_PATH)/guest/goldfish-opengl/shared/OpenglCodecCommon \
    $(EMUGL_PATH)/guest/goldfish-opengl/system/include \

goldfish_opengl_STATIC_LIBRARIES := \
	android-emu-base \

$(call emugl-begin-static-library,lib$(BUILD_TARGET_SUFFIX)Android)

androidSrcDir := androidImpl

LOCAL_C_INCLUDES := $(goldfish_opengl_INCLUDES)

LOCAL_SRC_FILES := \
	$(androidSrcDir)/Log.cpp \
	$(androidSrcDir)/Mutex.cpp \

$(call emugl-end-module)

$(call emugl-begin-static-library,lib$(BUILD_TARGET_SUFFIX)OpenglCodecCommonGuest)

codecCommonSrcDir := $(goldfish_opengl_DIR)/shared/OpenglCodecCommon

LOCAL_CFLAGS += $(goldfish_opengl_CFLAGS) -DLOG_TAG=\"eglCodecCommon\"

LOCAL_C_INCLUDES := $(goldfish_opengl_INCLUDES)

LOCAL_SRC_FILES := \
    $(codecCommonSrcDir)/GLClientState.cpp \
    $(codecCommonSrcDir)/GLESTextureUtils.cpp \
    $(codecCommonSrcDir)/ChecksumCalculator.cpp \
    $(codecCommonSrcDir)/GLSharedGroup.cpp \
    $(codecCommonSrcDir)/glUtils.cpp \
    $(codecCommonSrcDir)/IndexRangeCache.cpp \
    $(codecCommonSrcDir)/SocketStream.cpp \
    $(codecCommonSrcDir)/TcpStream.cpp \

$(call emugl-end-module)

### GLESv1_enc Encoder ###########################################
$(call emugl-begin-shared-library,libGLESv1_enc)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)Android)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglCodecCommonGuest)

LOCAL_STATIC_LIBRARIES += android-emu-base

glesv1EncDir := $(goldfish_opengl_DIR)/system/GLESv1_enc

LOCAL_CFLAGS += $(goldfish_opengl_CFLAGS) -DLOG_TAG=\"emuglGLESv1_enc\"

LOCAL_C_INCLUDES := $(goldfish_opengl_INCLUDES)

LOCAL_SRC_FILES := \
    $(glesv1EncDir)/GLEncoder.cpp \
    $(glesv1EncDir)/GLEncoderUtils.cpp \
    $(glesv1EncDir)/gl_client_context.cpp \
    $(glesv1EncDir)/gl_enc.cpp \
    $(glesv1EncDir)/gl_entry.cpp

$(call emugl-end-module)

### GLESv2_enc Encoder ###########################################
$(call emugl-begin-shared-library,libGLESv2_enc)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)Android)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglCodecCommonGuest)

LOCAL_STATIC_LIBRARIES += android-emu-base

glesv2EncDir := $(goldfish_opengl_DIR)/system/GLESv2_enc

LOCAL_CFLAGS += $(goldfish_opengl_CFLAGS) -DLOG_TAG=\"emuglGLESv2_enc\"

LOCAL_C_INCLUDES := \
	$(goldfish_opengl_INCLUDES) \
    $(EMUGL_PATH)/guest/goldfish-opengl/system/GLESv2_enc \

LOCAL_SRC_FILES := \
    $(glesv2EncDir)/GL2EncoderUtils.cpp \
    $(glesv2EncDir)/GL2Encoder.cpp \
    $(glesv2EncDir)/GLESv2Validation.cpp \
    $(glesv2EncDir)/gl2_client_context.cpp \
    $(glesv2EncDir)/gl2_enc.cpp \
    $(glesv2EncDir)/gl2_entry.cpp \
    $(glesv2EncDir)/../enc_common/IOStream_common.cpp \

$(call emugl-end-module)

### renderControl_enc Encoder ####################################

$(call emugl-begin-shared-library,lib_renderControl_enc)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)Android)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglCodecCommonGuest)

renderControlEncDir := $(goldfish_opengl_DIR)/system/renderControl_enc

LOCAL_STATIC_LIBRARIES += android-emu-base

LOCAL_C_INCLUDES := \
	$(goldfish_opengl_INCLUDES) \

LOCAL_SRC_FILES := \
    $(renderControlEncDir)/renderControl_client_context.cpp \
    $(renderControlEncDir)/renderControl_enc.cpp \
    $(renderControlEncDir)/renderControl_entry.cpp

$(call emugl-end-module)

### openglSystemCommon library ###################################

$(call emugl-begin-shared-library,libOpenglSystemCommon)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)Android)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglCodecCommonGuest)
$(call emugl-import,libGLESv1_enc libGLESv2_enc lib_renderControl_enc)

LOCAL_CFLAGS += $(goldfish_opengl_CFLAGS) -DHOST_BUILD

openglSystemCommonSrcDir := $(goldfish_opengl_DIR)/system/OpenglSystemCommon

LOCAL_C_INCLUDES := \
	$(goldfish_opengl_INCLUDES) \
    $(EMUGL_PATH)/guest/goldfish-opengl/system/GLESv1_enc \
    $(EMUGL_PATH)/guest/goldfish-opengl/system/GLESv2_enc \
    $(EMUGL_PATH)/guest/goldfish-opengl/system/renderControl_enc \

LOCAL_SRC_FILES := \
    $(openglSystemCommonSrcDir)/goldfish_dma.cpp \
    $(openglSystemCommonSrcDir)/FormatConversions.cpp \
    $(openglSystemCommonSrcDir)/HostConnection.cpp \
    $(openglSystemCommonSrcDir)/ProcessPipe.cpp    \
    $(openglSystemCommonSrcDir)/QemuPipeStream.cpp \
    $(openglSystemCommonSrcDir)/ThreadInfo.cpp

$(call emugl-end-module)

### GLESv1 implementation ########################################

$(call emugl-begin-shared-library,libGLESv1_CM_emulation)
$(call emugl-import,libOpenglSystemCommon libGLESv1_enc lib_renderControl_enc)

LOCAL_CFLAGS += $(goldfish_opengl_CFLAGS) -DLOG_TAG=\"GLES_emulation\" -DGL_GLEXT_PROTOTYPES

LOCAL_C_INCLUDES := \
	$(goldfish_opengl_INCLUDES) \
    $(EMUGL_PATH)/guest/goldfish-opengl/system/GLESv1_enc \
    $(EMUGL_PATH)/guest/goldfish-opengl/system/renderControl_enc \
    $(EMUGL_PATH)/guest/goldfish-opengl/system/OpenglSystemCommon \

LOCAL_SRC_FILES := $(goldfish_opengl_DIR)/system/GLESv1/gl.cpp

$(call emugl-end-module)
