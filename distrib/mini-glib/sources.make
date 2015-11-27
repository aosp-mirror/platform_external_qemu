# Included from top-level Makefile.common

MINI_GLIB_INCLUDE_DIR := $(LOCAL_PATH)/$(MINI_GLIB_DIR)/include

MINI_GLIB_SOURCES := \
    $(MINI_GLIB_DIR)/src/glib-mini.c \

ifeq ($(HOST_OS),windows)
MINI_GLIB_SOURCES += \
    $(MINI_GLIB_DIR)/src/glib-mini-win32.c \

endif
