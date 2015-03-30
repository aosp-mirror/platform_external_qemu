###############################################################################
###############################################################################
###
###  GLES_v1tov2 transfomer library
###
###  GLES 1.x to 2.0 transformer library
###  

LOCAL_PATH := $(call my-dir)

host_common_SRC_FILES :=           	\
    es1xColor.c             		\
    es1xContext.c               	\
    es1xDebug.c             		\
    es1xDefs.c              		\
    es1xEnumerations.c          	\
    es1xError.c             		\
    es1xLight.c             		\
    es1xMain.c              		\
    es1xMath.c              		\
    es1xMatrix.c                	\
    es1xMatrixStack.c           	\
    es1xMemory.c                	\
    es1xRasterizer.c            	\
    es1xRect.c              		\
    es1xShaderCache.c           	\
    es1xShaderContext.c         	\
    es1xShaderContextKey.c          	\
    es1xShaderPrograms.c            	\
    es1xStateQuery.c            	\
    es1xStateQueryLookup.c          	\
    es1xStateSetters.c          	\
    es1xStatistics.c            	\
    es1xTexEnv.c                	\
    es1xTexture.c               	\
    es1xTypeConverter.c         	\
    es1xVector.c			\
    es1xShaderProgramInfoHashTable.c	\
    es1xTextureHashTable.c		\
    es1xRenderFrameBuffer.c		\
    es1xRouting.c

host_common_CFLAGS := -ggdb -DDE_DEBUG -DES1X_SUPPORT_STATISTICS 

### GLES 1.x to 2.0 transformer  ############
$(call emugl-begin-host-static-library,libes1x)
$(call emugl-import,libDebase libGLcommon libGLESv2_dec)
LOCAL_SRC_FILES := $(host_common_SRC_FILES)
LOCAL_CFLAGS    += $(host_common_CFLAGS)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
$(call emugl-end-module)

### GLES 1.x to 2.0 transformer, 64-bit  ############
$(call emugl-begin-host64-static-library,lib64es1x)
$(call emugl-import,lib64Debase lib64GLcommon lib64GLESv2_dec)
LOCAL_SRC_FILES := $(host_common_SRC_FILES)
LOCAL_CFLAGS    += $(host_common_CFLAGS)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
$(call emugl-end-module)
