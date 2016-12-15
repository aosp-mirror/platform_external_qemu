# Rules to build QEMU2 for the Android emulator from sources.
# This file is included from external/qemu/Makefile or one of its sub-Makefiles.

# NOTE: Previous versions of the build system defined HOST_OS and HOST_ARCH
# instead of BUILD_TARGET_OS and BUILD_TARGET_ARCH, so take care of these to
# ensure this continues to build with both versions.
#
# TODO: Remove this once the external/qemu build script changes are
#       all submitted.
BUILD_TARGET_OS ?= $(HOST_OS)
BUILD_TARGET_ARCH ?= $(HOST_ARCH)
BUILD_TARGET_TAG ?= $(BUILD_TARGET_OS)-$(BUILD_TARGET_ARCH)
BUILD_TARGET_BITS ?= $(HOST_BITS)

# Same for OBJS_DIR -> BUILD_OBJS_DIR
BUILD_OBJS_DIR ?= $(OBJS_DIR)

QEMU2_OLD_LOCAL_PATH := $(LOCAL_PATH)

# Assume this is under android-qemu2-glue/build/ so get up two directories
# to find the top-level one.
LOCAL_PATH := $(QEMU2_TOP_DIR)

# If $(BUILD_TARGET_OS) is $1, then return $2, otherwise $3
qemu2-if-os = $(if $(filter $1,$(BUILD_TARGET_OS)),$2,$3)

# If $(BUILD_TARGET_ARCH) is $1, then return $2, otherwise $3
qemu2-if-build-target-arch = $(if $(filter $1,$(BUILD_TARGET_ARCH)),$2,$3)

# If $(BUILD_TARGET_OS) is not $1, then return $2, otherwise $3
qemu2-ifnot-os = $(if $(filter-out $1,$(BUILD_TARGET_OS)),$2,$3)

# If $(BUILD_TARGET_OS) is windows, return $1, otherwise $2
qemu2-if-windows = $(call qemu2-if-os,windows,$1,$2)

# If $(BUILD_TARGET_OS) is not windows, return $1, otherwise $2
qemu2-if-posix = $(call qemu2-ifnot-os,windows,$1,$2)

qemu2-if-darwin = $(call qemu2-if-os,darwin,$1,$2)
qemu2-if-linux = $(call qemu2-if-os,linux,$1,$2)

QEMU2_AUTO_GENERATED_DIR := $(BUILD_OBJS_DIR)/build/qemu2-qapi-auto-generated

QEMU2_DEPS_TOP_DIR := $(QEMU2_DEPS_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)
QEMU2_DEPS_LDFLAGS := -L$(QEMU2_DEPS_TOP_DIR)/lib

QEMU2_GLIB_INCLUDES := $(QEMU2_DEPS_TOP_DIR)/include/glib-2.0 \
                       $(QEMU2_DEPS_TOP_DIR)/lib/glib-2.0/include

QEMU2_GLIB_LDLIBS := \
    -lglib-2.0 \
    $(call qemu2-if-darwin, -liconv -lintl) \
    $(call qemu2-if-linux, -lpthread -lrt) \
    $(call qemu2-if-windows, -lole32) \

QEMU2_PIXMAN_INCLUDES := $(QEMU2_DEPS_TOP_DIR)/include/pixman-1
QEMU2_PIXMAN_LDLIBS := -lpixman-1

QEMU2_SDL2_INCLUDES := $(QEMU2_DEPS_TOP_DIR)/include/SDL2
QEMU2_SDL2_LDLIBS := \
    $(call qemu2-if-windows, \
        -lmingw32 \
    ) \
    -lSDL2 \
    $(call qemu2-if-darwin,, \
        -lSDL2main \
    ) \
    $(call qemu2-if-windows, \
        -limm32 \
        -ldinput8 \
        -ldxguid \
        -ldxerr8 \
        -luser32 \
        -lgdi32 \
        -lwinmm \
        -lole32 \
        -loleaut32 \
        -lshell32 \
        -lversion \
        -luuid \
    , \
        -ldl \
    ) \

ifeq (darwin,$(BUILD_TARGET_OS))
# NOTE: Because the following contain commas, we cannot use qemu2-if-darwin!
QEMU2_SDL2_LDLIBS += \
    -Wl,-framework,OpenGL \
    -Wl,-framework,ForceFeedback \
    -lobjc \
    -Wl,-framework,Cocoa \
    -Wl,-framework,Carbon \
    -Wl,-framework,IOKit \
    -Wl,-framework,CoreAudio \
    -Wl,-framework,AudioToolbox \
    -Wl,-framework,AudioUnit \

endif

# Ensure config-host.h can be found properly.
QEMU2_INCLUDES := $(LOCAL_PATH)/android-qemu2-glue/config/$(BUILD_TARGET_TAG)

# Ensure QEMU2 headers are found.
QEMU2_INCLUDES += \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/include \
    $(QEMU2_AUTO_GENERATED_DIR) \

QEMU2_INCLUDES += $(QEMU2_GLIB_INCLUDES) $(QEMU2_PIXMAN_INCLUDES)

QEMU2_CFLAGS := \
    $(EMULATOR_COMMON_CFLAGS) \
    $(call qemu2-if-windows,\
        -DWIN32_LEAN_AND_MEAN \
        -D__USE_MINGW_ANSI_STDIO=1 \
        -mms-bitfields) \
    -fno-strict-aliasing \
    -fno-common \
    $(LIBCURL_CFLAGS) \
    -D_GNU_SOURCE \
    -D_FILE_OFFSET_BITS=64 \
    $(call qemu2-if-darwin, -Wno-initializer-overrides) \

QEMU2_CFLAGS += \
    -Wno-unused-function \
    $(call qemu2-if-darwin, \
        -Wno-unused-value \
        -Wno-parentheses-equality \
        -Wno-self-assign \
        , \
        -Wno-unused-variable \
        -Wno-unused-but-set-variable \
        -Wno-maybe-uninitialized \
        ) \

#include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-glue.mk

#include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-qt.mk

include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-sources.mk

# A static library containing target-independent code
$(call start-emulator-library,libqemu2-common)

LOCAL_CFLAGS += \
    $(QEMU2_CFLAGS) \
    -DPOISON_CONFIG_ANDROID \

LOCAL_C_INCLUDES += \
    $(QEMU2_INCLUDES) \
    $(LOCAL_PATH)/slirp \
    $(LOCAL_PATH)/tcg \
    $(ZLIB_INCLUDES) \

LOCAL_SRC_FILES += \
    $(QEMU2_COMMON_SOURCES) \
    $(QEMU2_COMMON_SOURCES_$(BUILD_TARGET_TAG))

LOCAL_GENERATED_SOURCES += \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-event.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-types.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-visit.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qmp-introspect.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qmp-marshal.c \
    $(QEMU2_AUTO_GENERATED_DIR)/trace/generated-events.c \
    $(QEMU2_AUTO_GENERATED_DIR)/trace/generated-tracers.c \

# Stuff from libqemuutil, order follows util/Makefile.objs
LOCAL_SRC_FILES += \
    crypto/pbkdf-stub.c \
    qapi/opts-visitor.c \
    qapi/qapi-clone-visitor.c \
    qapi/qapi-dealloc-visitor.c \
    qapi/qapi-util.c \
    qapi/qapi-visit-core.c \
    qapi/qmp-dispatch.c \
    qapi/qmp-event.c \
    qapi/qmp-input-visitor.c \
    qapi/qmp-output-visitor.c \
    qapi/qmp-registry.c \
    qapi/string-input-visitor.c \
    qapi/string-output-visitor.c \
    qobject/json-lexer.c \
    qobject/json-parser.c \
    qobject/json-streamer.c \
    qobject/qbool.c \
    qobject/qdict.c \
    qobject/qfloat.c \
    qobject/qint.c \
    qobject/qjson.c \
    qobject/qlist.c \
    qobject/qnull.c \
    qobject/qobject.c \
    qobject/qstring.c \
    trace/control.c \
    trace/qmp.c \
    util/abort.c \
    util/acl.c \
    util/base64.c \
    util/bitmap.c \
    util/bitops.c \
    util/buffer.c \
    util/crc32c.c \
    util/cutils.c \
    util/envlist.c \
    util/error.c \
    util/fifo8.c \
    util/getauxval.c \
    util/hexdump.c \
    util/hbitmap.c \
    util/id.c \
    util/iov.c \
    util/log.c \
    util/module.c \
    util/notify.c \
    util/osdep.c \
    util/path.c \
    util/qdist.c \
    util/qemu-config.c \
    util/qemu-coroutine.c \
    util/qemu-coroutine-io.c \
    util/qemu-coroutine-lock.c \
    util/qemu-coroutine-sleep.c \
    util/qemu-error.c \
    util/qemu-option.c \
    util/qemu-progress.c \
    util/qemu-sockets.c \
    util/qemu-timer-common.c \
    util/qht.c \
    util/range.c \
    util/rcu.c \
    util/readline.c \
    util/rfifolock.c \
    util/timed-average.c \
    util/throttle.c \
    util/unicode.c \
    util/uri.c \
    $(call qemu2-if-windows, \
        util/coroutine-win32.c \
        util/event_notifier-win32.c \
        util/oslib-win32.c \
        util/qemu-thread-win32.c \
        ) \
    $(call qemu2-if-linux, \
        util/coroutine-ucontext.c \
        util/memfd.c \
        ) \
    $(call qemu2-if-darwin, \
        util/coroutine-sigaltstack.c \
        ) \
    $(call qemu2-if-posix, \
        util/event_notifier-posix.c \
        util/mmap-alloc.c \
        util/oslib-posix.c \
        util/qemu-openpty.c \
        util/qemu-thread-posix.c \
        ) \
    $(call qemu2-if-build-target-arch,x86, util/host-utils.c) \
    $(call qemu2-if-posix, \
        util/compatfd.c \
        ) \

$(call gen-hw-config-defs)
QEMU2_INCLUDES += $(QEMU_HW_CONFIG_DEFS_INCLUDES)

$(call end-emulator-library)

include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-glue.mk

QEMU2_TARGET := x86
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

QEMU2_TARGET := x86_64
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

QEMU2_TARGET := arm
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

QEMU2_TARGET := arm64
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

QEMU2_TARGET := mips
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

QEMU2_TARGET := mips64
include $(LOCAL_PATH)/android-qemu2-glue/build/Makefile.qemu2-target.mk

LOCAL_PATH := $(QEMU2_OLD_LOCAL_PATH)
