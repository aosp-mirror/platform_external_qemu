# Rules to build QEMU2 for the Android emulator from sources.
# This file is included from external/qemu/Makefile or one of its sub-Makefiles.

QEMU2_OLD_LOCAL_PATH := $(LOCAL_PATH)

# Assume this is under android-qemu2-glue/build/ so get up two directories
# to find the top-level one.
LOCAL_PATH := $(QEMU2_TOP_DIR)

# If $(HOST_OS) is $1, then return $2, otherwise $3
qemu2-if-os = $(if $(filter $1,$(HOST_OS)),$2,$3)

# If $(HOST_ARCH) is $1, then return $2, otherwise $3
qemu2-if-host-arch = $(if $(filter $1,$(HOST_ARCH)),$2,$3)

# If $(HOST_OS) is not $1, then return $2, otherwise $3
qemu2-ifnot-os = $(if $(filter-out $1,$(HOST_OS)),$2,$3)

# If $(HOST_OS) is windows, return $1, otherwise $2
qemu2-if-windows = $(call qemu2-if-os,windows,$1,$2)

# If $(HOST_OS) is not windows, return $1, otherwise $2
qemu2-if-posix = $(call qemu2-ifnot-os,windows,$1,$2)

qemu2-if-darwin = $(call qemu2-if-os,darwin,$1,$2)
qemu2-if-linux = $(call qemu2-if-os,linux,$1,$2)

QEMU2_AUTO_GENERATED_DIR := $(OBJS_DIR)/build/qemu2-qapi-auto-generated

QEMU2_DEPS_TOP_DIR := $(QEMU2_DEPS_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)
QEMU2_DEPS_LDFLAGS := -L$(QEMU2_DEPS_TOP_DIR)/lib

QEMU2_GLIB_INCLUDES := $(QEMU2_DEPS_TOP_DIR)/include/glib-2.0 \
                       $(QEMU2_DEPS_TOP_DIR)/lib/glib-2.0/include

# make sure we set up the AndroidEmu include path
ANDROID_EMULIB_INCLUDES := $(LOCAL_PATH)/../qemu/

QEMU2_GLIB_LDLIBS := \
    -lglib-2.0 \
    $(call qemu2-if-darwin, -liconv -lintl) \
    $(call qemu2-if-linux, -lpthread -lrt)

QEMU2_PIXMAN_INCLUDES := $(QEMU2_DEPS_TOP_DIR)/include/pixman-1
QEMU2_PIXMAN_LDLIBS := -lpixman-1

# Ensure config-host.h can be found properly.
QEMU2_INCLUDES := $(LOCAL_PATH)/android-qemu2-glue/config/$(HOST_OS)-$(HOST_ARCH)
# Ensure QEMU2 headers are found.
QEMU2_INCLUDES += \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/include \
    $(QEMU2_AUTO_GENERATED_DIR) \
    $(LIBEXT4_UTILS_INCLUDES) \

ifndef EMULATOR_USE_SDL2
include $(QEMU2_OLD_LOCAL_PATH)/distrib/libsdl2.mk
endif
QEMU2_SDL2_INCLUDES := $(SDL2_INCLUDES)/SDL2

QEMU2_INCLUDES += $(QEMU2_GLIB_INCLUDES) $(QEMU2_PIXMAN_INCLUDES)

QEMU2_CFLAGS := \
    $(call qemu2-if-windows,\
        -DWIN32_LEAN_AND_MEAN \
        -D__USE_MINGW_ANSI_STDIO=1 \
        -mms-bitfields) \
    -fno-strict-aliasing \
    -fno-common \
    $(LIBCURL_CFLAGS) \
    -D_GNU_SOURCE \
    -D_FILE_OFFSET_BITS=64 \
    -DCONFIG_ANDROID \
    -DUSE_ANDROID_EMU \

include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-glue.mk

ifdef EMULATOR_USE_QT
    include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-qt.mk
endif

include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-sources.mk

# Custom fixes.
QEMU2_COMMON_SOURCES += \
    stubs/kvm.c

# A static library containing target-independent code
$(call start-emulator-library,libqemu2_common)

LOCAL_CFLAGS += $(QEMU2_CFLAGS)

LOCAL_C_INCLUDES += \
    $(QEMU2_INCLUDES) \
    $(QEMU2_SDL2_INCLUDES) \
    $(LOCAL_PATH)/slirp \
    $(LOCAL_PATH)/tcg \
    $(SDL2_INCLUDES) \
    $(ANDROID_EMULIB_INCLUDES)

LOCAL_SRC_FILES += \
    $(QEMU2_COMMON_SOURCES) \
    $(QEMU2_COMMON_SOURCES_$(HOST_OS)-$(HOST_ARCH))

LOCAL_GENERATED_SOURCES += \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-event.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-types.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-visit.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qmp-marshal.c \
    $(QEMU2_AUTO_GENERATED_DIR)/trace/generated-events.c \
    $(QEMU2_AUTO_GENERATED_DIR)/trace/generated-tracers.c \

# Stuff from libqemuutil, order follows util/Makefile.objs
LOCAL_SRC_FILES += \
    qapi/qapi-visit-core.c \
    qapi/qapi-dealloc-visitor.c \
    qapi/qmp-input-visitor.c \
    qapi/qmp-output-visitor.c \
    qapi/qmp-registry.c \
    qapi/qmp-dispatch.c \
    qapi/string-input-visitor.c \
    qapi/string-output-visitor.c \
    qapi/opts-visitor.c \
    qapi/qmp-event.c \
    qapi/qapi-util.c \
    qobject/qint.c \
    qobject/qstring.c \
    qobject/qdict.c \
    qobject/qlist.c \
    qobject/qfloat.c \
    qobject/qbool.c \
    qobject/qjson.c \
    qobject/json-lexer.c \
    qobject/json-streamer.c \
    qobject/json-parser.c \
    qobject/qerror.c \
    trace/control.c \
    trace/qmp.c \
    util/osdep.c \
    util/cutils.c \
    util/unicode.c \
    util/qemu-timer-common.c \
    $(call qemu2-if-windows, \
        util/oslib-win32.c \
        util/qemu-thread-win32.c \
        util/event_notifier-win32.c \
        ) \
    $(call qemu2-if-posix, \
        util/oslib-posix.c \
        util/qemu-thread-posix.c \
        util/event_notifier-posix.c \
        util/qemu-openpty.c \
        ) \
    util/envlist.c \
    util/path.c \
    util/module.c \
    $(call qemu2-if-host-arch,x86, util/host-utils.c) \
    util/bitmap.c \
    util/bitops.c \
    util/hbitmap.c \
    util/fifo8.c \
    util/acl.c \
    util/error.c \
    util/qemu-error.c \
    $(call qemu2-if-posix, \
        util/compatfd.c \
        ) \
    util/id.c \
    util/iov.c \
    util/aes.c \
    util/qemu-config.c \
    util/qemu-sockets.c \
    util/uri.c \
    util/notify.c \
    util/qemu-option.c \
    util/qemu-progress.c \
    util/hexdump.c \
    util/crc32c.c \
    util/throttle.c \
    util/getauxval.c \
    util/readline.c \
    util/rfifolock.c \
    $(call qemu2-if-windows, \
        util/shared-library-win32.c \
        ) \
    $(call qemu2-if-posix, \
        util/shared-library-posix.c \
        ) \

$(call gen-hw-config-defs)
QEMU2_INCLUDES += $(QEMU_HW_CONFIG_DEFS_INCLUDES)

ifdef EMULATOR_USE_QT
    # everything needed to build Qt UI
    LOCAL_CFLAGS += \
        $(ANDROID_SKIN_CFLAGS)

    LOCAL_SRC_FILES += \
        $(ANDROID_SKIN_SOURCES:%=../qemu/%) \
        $(QEMU2_GLUE_SOURCES)

    LOCAL_QT_MOC_SRC_FILES := $(ANDROID_SKIN_QT_MOC_SRC_FILES:%=../qemu/%)
    LOCAL_QT_RESOURCES := $(ANDROID_SKIN_QT_RESOURCES:%=../qemu/%)
    LOCAL_QT_UI_SRC_FILES := $(ANDROID_SKIN_QT_UI_SRC_FILES:%=../qemu/%)
endif

$(call end-emulator-library)

# Special case, the following sources are only used by the arm64
# target but cannot be part of libqemu2_aarch64 because they need
# to be compiled without NEED_CPU_H
$(call start-emulator-library,libqemu2_common_aarch64)

LOCAL_CPP_EXTENSION := .cc

LOCAL_C_INCLUDES += \
    $(QEMU2_INCLUDES) \
    $(LOCAL_PATH)/target-arm \
    $(LOCAL_PATH)/disas/libvixl

LOCAL_CFLAGS += $(QEMU2_CFLAGS)

LOCAL_SRC_FILES += \
    disas/arm-a64.cc \
    disas/libvixl/a64/decoder-a64.cc \
    disas/libvixl/a64/disasm-a64.cc \
    disas/libvixl/a64/instructions-a64.cc \
    disas/libvixl/utils.cc \

$(call end-emulator-library)

QEMU2_TARGET := x86
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

QEMU2_TARGET := x86_64
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

QEMU2_TARGET := arm64
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

QEMU2_TARGET := mips64
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

LOCAL_PATH := $(QEMU2_OLD_LOCAL_PATH)
