# Build rules for the static breakpad prebuilt libraries.

WEBSOCKETS_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

WEBSOCKETS_TOP_DIR := $(LOCAL_PATH)/build/

WEBSOCKETS_INCLUDES := $(LOCAL_PATH)/lib
WEBSOCKETS_LDLIBS :=

$(call start-emulator-library,emulator-libwebsockets)

#WEBSOCKETS_STATIC_LIBRARIES := \
#    emulator-libwebsockets
LOCAL_SRC_FILES := \
	lib/alloc.c \
	lib/base64-decode.c \
	lib/context.c \
	lib/client.c \
	lib/client-handshake.c \
	lib/client-parser.c \
	lib/extension-permessage-deflate.c \
	lib/extension.c \
	lib/fops-zip.c \
	lib/handshake.c \
	lib/header.c \
	lib/libwebsockets.c \
	lib/output.c \
	lib/parsers.c \
	lib/pollfd.c \
	lib/ranges.c \
	lib/server-handshake.c \
	lib/server.c \
	lib/service.c \
	lib/sha-1.c \

WEBSOCKETS_INCLUDES += $(LOCAL_PATH)/include

ifeq ($(BUILD_TARGET_OS),windows)
  LOCAL_SRC_FILES += lib/lws-plat-win.c \
		     win32port/win32helpers/gettimeofday.c \
		     win32port/win32helpers/getopt_long.c \
		     win32port/win32helpers/getopt.c
  LOCAL_C_INCLUDES += $(LOCAL_PATH)/include \
		      $(LOCAL_PATH)/win32port/win32helpers/ \
		      $(LOCAL_PATH)/win32port/zlib/
  LOCAL_CFLAGS += -Wno-int-to-pointer-cast \
		  -Wno-pointer-to-int-cast 
else
  LOCAL_SRC_FILES += lib/lws-plat-unix.c
  LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
endif

ifeq ($(BUILD_TARGET_OS),darwin)
  LOCAL_CFLAGS += -fvisibility=hidden \
		  -Wno-deprecated-declarations
endif

LOCAL_CFLAGS += \
    -O3 \
    -fstrict-aliasing \
    -Wno-all \
    -nostdlib \
    -D__USE_MINGW_ANSI_STDIO

$(call end-emulator-library)

LOCAL_PATH := $(WEBSOCKETS_OLD_LOCAL_PATH)
