# Included from top-level Makefile.common

MINIGLIB_DIR := $(call my-dir)

MINIGLIB_INCLUDES := $(MINIGLIB_DIR)/include

MINIGLIB_SOURCES := \
    $(MINIGLIB_DIR)/src/glib-mini.c \

ifeq ($(BUILD_TARGET_OS),windows)
MINIGLIB_SOURCES += \
    $(MINIGLIB_DIR)/src/glib-mini-win32.c \

endif
