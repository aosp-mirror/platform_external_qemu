##############################################################################
##############################################################################
###
###  emulator-$ARCH: Standalone launcher for QEMU executable.
###
###

# Check EMULATOR_TARGET_CPU value and determine
# corresponding TARGET_XXX macro at the same time

EMULATOR_TARGET_CPU := $(EMULATOR_TARGET_ARCH)

TARGET_VARNAME_arm64  := TARGET_ARM64
TARGET_VARNAME_mips64 := TARGET_MIPS64
TARGET_VARNAME_x86_64 := TARGET_X86_64
TARGET_VARNAME_x86    := TARGET_X86
ifeq (,$(strip $(TARGET_VARNAME_$(EMULATOR_TARGET_CPU))))
  $(error Unrecognized EMULATOR_TARGET_CPU value: [$(EMULATOR_TARGET_CPU)])
endif

qemu_launcher_SOURCES := \
    android/qemu-launcher/emulator-qemu.cpp \
    android/cmdline-option.c \
    android/cpu_accelerator.cpp \
    android/help.c \
    android/main-common.c \

qemu_launcher_CFLAGS := \
    -DNO_SKIN=1 \
    -D$(TARGET_VARNAME_$(EMULATOR_TARGET_CPU))=1 \
    -I$(OBJS_DIR)/build

qemu_launcher_LDLIBS := $(CXX_STD_LIB)

$(call start-emulator-program, emulator$(HOST_SUFFIX)-ranchu-$(EMULATOR_TARGET_CPU))
LOCAL_SRC_FILES := $(qemu_launcher_SOURCES)
LOCAL_CFLAGS := $(qemu_launcher_CFLAGS)
LOCAL_STATIC_LIBRARIES := \
    emulator-common \
    emulator-libext4_utils \
    emulator-libsparse \
    emulator-libselinux \
    emulator-zlib
LOCAL_LDLIBS := $(qemu_launcher_LDLIBS)
LOCAL_GENERATE_SYMBOLS := true
$(call gen-hw-config-defs)

ifeq ($(HOST_OS),windows)
$(eval $(call insert-windows-icon))
endif

$(call end-emulator-program)
