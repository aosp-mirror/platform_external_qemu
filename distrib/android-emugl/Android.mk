# This is the top-level build file for the Android HW OpenGL ES emulation
# in Android.
#
# You must define BUILD_EMULATOR_HOST_OPENGL to 'true' in your environment to
# build the following files.
#
ifeq (true,$(BUILD_EMULATOR_HOST_OPENGL))

# Top-level for all modules
EMUGL_PATH := $(call my-dir)

# Directory containing common headers used by several modules
# This is always set to a module's LOCAL_C_INCLUDES
# See the definition of emugl-begin-module in common.mk
#
EMUGL_COMMON_INCLUDES := \
    $(EMUGL_PATH)/host/include/libOpenglRender \
    $(EMUGL_PATH)/shared

ifeq ($(BUILD_STANDALONE_EMULATOR),true)
EMUGL_COMMON_INCLUDES := $(EMUGL_PATH)/host/libs/Translator/include
endif

# common cflags used by several modules
# This is always set to a module's LOCAL_CFLAGS
# See the definition of emugl-begin-module in common.mk

# Needed to ensure SIZE_MAX is properly defined when including <stdint.h>
EMUGL_COMMON_CFLAGS += -D__STDC_LIMIT_MACROS=1

# Define EMUGL_BUILD_DEBUG=1 in your environment to build a
# debug version of the EmuGL host binaries.
ifneq (,$(strip $(EMUGL_BUILD_DEBUG)))
EMUGL_COMMON_CFLAGS += -O0 -g -DEMUGL_DEBUG=1
endif

EMUGL_COMMON_CFLAGS += -DEMUGL_BUILD=1
ifeq (linux,$(HOST_OS))
EMUGL_COMMON_CFLAGS += -fvisibility=internal
endif
ifeq (darwin,$(HOST_OS))
EMUGL_COMMON_CFLAGS += -fvisibility=hidden
endif

# Uncomment the following line if you want to enable debug traces
# in the GLES emulation libraries.
# EMUGL_COMMON_CFLAGS += -DEMUGL_DEBUG=1

# Include common definitions used by all the modules included later
# in this build file. This contains the definition of all useful
# emugl-xxxx functions.
#
include $(EMUGL_PATH)/common.mk

# IMPORTANT: ORDER IS CRUCIAL HERE
#
# For the import/export feature to work properly, you must include
# modules below in correct order. That is, if module B depends on
# module A, then it must be included after module A below.
#
# This ensures that anything exported by module A will be correctly
# be imported by module B when it is declared.
#
# Note that the build system will complain if you try to import a
# module that hasn't been declared yet anyway.
#

# Required by our units test.
include $(EMUGL_PATH)/googletest.mk

# First, build the emugen host source-generation tool
#
# It will be used by other modules to generate wire protocol encode/decoder
# source files (see all emugl-gen-decoder/encoder in common.mk)
#
include $(EMUGL_PATH)/host/tools/emugen/Android.mk

include $(EMUGL_PATH)/shared/emugl/common/Android.mk
include $(EMUGL_PATH)/shared/OpenglCodecCommon/Android.mk

# Host static libraries
include $(EMUGL_PATH)/host/libs/GLESv1_dec/Android.mk
include $(EMUGL_PATH)/host/libs/GLESv2_dec/Android.mk
include $(EMUGL_PATH)/host/libs/renderControl_dec/Android.mk
include $(EMUGL_PATH)/host/libs/Translator/GLcommon/Android.mk
include $(EMUGL_PATH)/host/libs/Translator/GLES_CM/Android.mk
include $(EMUGL_PATH)/host/libs/Translator/GLES_V2/Android.mk
include $(EMUGL_PATH)/host/libs/Translator/EGL/Android.mk

# Host shared libraries
include $(EMUGL_PATH)/host/libs/libOpenglRender/Android.mk

endif # BUILD_EMULATOR_HOST_OPENGL == true
