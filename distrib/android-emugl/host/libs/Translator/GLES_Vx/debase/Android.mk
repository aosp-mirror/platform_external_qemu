################################################################################
################################################################################
###
###  dEQP debase library
###
###  The GLES 1.1 to 2.0 transformer depends on debase library in dEQP.
###  Since we need to build both 32-bit and 64-bit versions for our 
###  transformer, we define a local module of debase for our use.
###

LOCAL_PATH := $(call my-dir)

# Set OS (required to defined debugging flags)
ifeq ($(HOST_OS), linux)
    DE_OS := __unix__
else ifeq ($(HOST_OS), darwin)
    DE_OS := __APPLE__
else ifeq ($(HOST_OS), windows)
    DE_OS := _WIN32
endif
host_common_CFLAGS    := -O0 -D$(DE_OS) -ggdb

host_common_SRC_FILES :=	\
    deDefs.c       		\
    deFloat16.c     		\
    deInt32.c       		\
    deInt32Test.c       	\
    deMath.c        		\
    deMemory.c      		\
    deRandom.c      		\
    deString.c

$(call emugl-begin-host-static-library,libDebase)
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_CFLAGS += $(host_common_CFLAGS)
LOCAL_SRC_FILES  := $(host_common_SRC_FILES)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
# $(call emugl-export,SHARED_LIBRARIES,libmath)
$(call emugl-end-module)

$(call emugl-begin-host64-static-library,lib64Debase)
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_CFLAGS += $(host_common_CFLAGS)
LOCAL_SRC_FILES  := $(host_common_SRC_FILES)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
# $(call emugl-export,SHARED_LIBRARIES,lib64math)
$(call emugl-end-module)
