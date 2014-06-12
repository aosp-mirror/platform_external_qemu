# Included from top-level Makefile.common

GLIB_INCLUDE_DIR := $(LOCAL_PATH)/$(GLIB_DIR)/include

GLIB_SOURCES := \
    $(GLIB_DIR)/src/glib-mini.c \

ifeq ($(HOST_OS),windows)
GLIB_SOURCES += \
    $(GLIB_DIR)/src/glib-mini-win32.c \

endif
