##############################################################################
##############################################################################
###
###  emulator-$ARCH: Standalone launcher for QEMU executable.
###
###

# Check EMULATOR_TARGET_ARCH value and determine
# corresponding TARGET_XXX macro at the same time

TARGET_VARNAME_arm64  := TARGET_ARM64
TARGET_VARNAME_mips64 := TARGET_MIPS64
TARGET_VARNAME_mips   := TARGET_MIPS
TARGET_VARNAME_x86_64 := TARGET_X86_64
TARGET_VARNAME_x86    := TARGET_X86
ifeq (,$(strip $(TARGET_VARNAME_$(EMULATOR_TARGET_ARCH))))
  $(error Unrecognized EMULATOR_TARGET_ARCH value: [$(EMULATOR_TARGET_ARCH)])
endif

$(call start-emulator-program,\
    emulator$(HOST_SUFFIX)-ranchu-$(EMULATOR_TARGET_ARCH))

LOCAL_SRC_FILES := \
    android/qemu-launcher/emulator-qemu.cpp \
    android/cmdline-option.c \
    android/cpu_accelerator.cpp \
    android/help.c \
    android/main-common.c \

LOCAL_CFLAGS := \
    -DNO_SKIN=1 \
    -D$(TARGET_VARNAME_$(EMULATOR_TARGET_ARCH))=1 \

LOCAL_C_INCLUDES := $(OBJS_DIR)/build

LOCAL_STATIC_LIBRARIES := \
    $(ANDROID_EMU_STATIC_LIBRARIES_QEMU1) \
    emulator-common \
    emulator-libext4_utils \
    emulator-libsparse \
    emulator-libselinux \
    emulator-zlib

LOCAL_LDLIBS += $(CXX_STD_LIB)

LOCAL_GENERATE_SYMBOLS := true

$(call gen-hw-config-defs)

ifeq ($(HOST_OS),windows)
$(eval $(call insert-windows-icon))
endif

$(call end-emulator-program)
