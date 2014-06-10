###########################################################
###########################################################
###
###  ext4_utils and associated libraries.
###
###  ext4_utils depends on libsparse and (on POSIX platforms) libselinux.
###  It also depends on zlib which is compiled seperately.

ifeq ($(HOST_OS),windows)
    EMULATOR_EXT4_UTILS_CFLAGS += -DUSE_MINGW=1
else
EMULATOR_SELINUX_SOURCES_DIR ?= $(LOCAL_PATH)/../libselinux
EMULATOR_SELINUX_SOURCES_DIR := $(EMULATOR_SELINUX_SOURCES_DIR)
ifeq (,$(strip $(wildcard $(EMULATOR_SELINUX_SOURCES_DIR))))
    $(error Cannot find libselinux sources directory: $(EMULATOR_SELINUX_SOURCES_DIR))
endif

EMULATOR_SELINUX_INCLUDES := $(EMULATOR_SELINUX_SOURCES_DIR)/include
EMULATOR_SELINUX_SOURCES := src/callbacks.c \
    src/check_context.c \
    src/freecon.c \
    src/init.c \
    src/label.c \
    src/label_file.c \
    src/label_android_property.c

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(EMULATOR_SELINUX_SOURCES_DIR)

$(call start-emulator-library, emulator-libselinux)
LOCAL_C_INCLUDES += $(EMULATOR_SELINUX_INCLUDES)
LOCAL_SRC_FILES := $(EMULATOR_SELINUX_SOURCES)
$(call end-emulator-library)

ifdef EMULATOR_BUILD_64BITS
    $(call start-emulator-library, emulator64-libselinux)
    LOCAL_C_INCLUDES += $(EMULATOR_SELINUX_INCLUDES)
    LOCAL_CFLAGS += -m64
    LOCAL_SRC_FILES := $(EMULATOR_SELINUX_SOURCES)
    $(call end-emulator-library)
endif

LOCAL_PATH := $(old_LOCAL_PATH)
endif


EMULATOR_LIBSPARSE_SOURCES_DIR ?= $(LOCAL_PATH)/../../system/core/libsparse
EMULATOR_LIBSPARSE_SOURCES_DIR := $(EMULATOR_LIBSPARSE_SOURCES_DIR)
ifeq (,$(strip $(wildcard $(EMULATOR_LIBSPARSE_SOURCES_DIR))))
    $(error Cannot find libsparse sources directory: $(EMULATOR_LIBSPARSE_SOURCES_DIR))
endif

EMULATOR_LIBSPARSE_INCLUDES := $(EMULATOR_LIBSPARSE_SOURCES_DIR)/include \
    $(LOCAL_PATH)/distrib/zlib-1.2.8
EMULATOR_LIBSPARSE_SOURCES := \
    backed_block.c \
    output_file.c \
    sparse.c \
    sparse_crc32.c \
    sparse_err.c \
    sparse_read.c

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(EMULATOR_LIBSPARSE_SOURCES_DIR)

$(call start-emulator-library, emulator-libsparse)
LOCAL_C_INCLUDES += $(EMULATOR_LIBSPARSE_INCLUDES)
LOCAL_CFLAGS += $(EMULATOR_EXT4_UTILS_CFLAGS)
LOCAL_SRC_FILES := $(EMULATOR_LIBSPARSE_SOURCES)
$(call end-emulator-library)

ifdef EMULATOR_BUILD_64BITS
    $(call start-emulator-library, emulator64-libsparse)
    LOCAL_C_INCLUDES += $(EMULATOR_LIBSPARSE_INCLUDES)
    LOCAL_CFLAGS += $(EMULATOR_EXT4_UTILS_CFLAGS) -m64
    LOCAL_SRC_FILES := $(EMULATOR_LIBSPARSE_SOURCES)
    $(call end-emulator-library)
endif

LOCAL_PATH := $(old_LOCAL_PATH)


EMULATOR_EXT4_UTILS_SOURCES_DIR ?= $(LOCAL_PATH)/../../system/extras/ext4_utils
EMULATOR_EXT4_UTILS_SOURCES_DIR := $(EMULATOR_EXT4_UTILS_SOURCES_DIR)
ifeq (,$(strip $(wildcard $(EMULATOR_EXT4_UTILS_SOURCES_DIR))))
    $(error Cannot find ext4_utils sources directory: $(EMULATOR_EXT4_UTILS_SOURCES_DIR))
endif

EMULATOR_EXT4_UTILS_INCLUDES := $(EMULATOR_EXT4_UTILS_SOURCES_DIR) \
    $(EMULATOR_LIBSPARSE_INCLUDES) \
    $(EMULATOR_SELINUX_INCLUDES) \
    $(EMULATOR_EXT4_UTILS_SOURCES_DIR)/../../core/include
EMULATOR_EXT4_UTILS_SOURCES := \
    make_ext4fs.c \
    ext4fixup.c \
    ext4_utils.c \
    allocate.c \
    contents.c \
    extent.c \
    indirect.c \
    uuid.c \
    sha1.c \
    wipe.c \
    crc16.c \
    ext4_sb.c

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(EMULATOR_EXT4_UTILS_SOURCES_DIR)

$(call start-emulator-library, emulator-ext4_utils)
LOCAL_C_INCLUDES += $(EMULATOR_EXT4_UTILS_INCLUDES)
LOCAL_CFLAGS += $(EMULATOR_EXT4_UTILS_CFLAGS)
LOCAL_SRC_FILES := $(EMULATOR_EXT4_UTILS_SOURCES)
$(call end-emulator-library)

ifdef EMULATOR_BUILD_64BITS
    $(call start-emulator-library, emulator64-ext4_utils)
    LOCAL_C_INCLUDES += $(EMULATOR_EXT4_UTILS_INCLUDES)
    LOCAL_CFLAGS += $(EMULATOR_EXT4_UTILS_CFLAGS) -m64
    LOCAL_SRC_FILES := $(EMULATOR_EXT4_UTILS_SOURCES)
    $(call end-emulator-library)
endif

LOCAL_PATH := $(old_LOCAL_PATH)

