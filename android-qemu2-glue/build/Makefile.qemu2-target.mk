# Rules to build the final target-specific QEMU2 binaries from sources.

# The QEMU-specific target name for a given emulator CPU architecture.
QEMU2_TARGET_CPU_x86 := i386
QEMU2_TARGET_CPU_x86_64 := x86_64
QEMU2_TARGET_CPU_mips := mipsel
QEMU2_TARGET_CPU_mips64 := mips64el
QEMU2_TARGET_CPU_arm := arm
QEMU2_TARGET_CPU_arm64 := aarch64

# The engine will use the sources from target-<name> to emulate a given
# emulator CPU architecture.
QEMU2_TARGET_TARGET_x86 := i386
QEMU2_TARGET_TARGET_x86_64 := i386
QEMU2_TARGET_TARGET_mips := mips
QEMU2_TARGET_TARGET_mips64 := mips
QEMU2_TARGET_TARGET_arm := arm
QEMU2_TARGET_TARGET_arm64 := arm

# As a special case, the 32-bit ARM binary is named qemu-system-armel to match
QEMU2_TARGET_CPU := $(QEMU2_TARGET_CPU_$(QEMU2_TARGET))
QEMU2_TARGET_TARGET := $(QEMU2_TARGET_TARGET_$(QEMU2_TARGET))

# upstream QEMU, even though the upstream configure script now uses the 'arm'
# target name (and stores the build files under arm-softmmu instead of
# armel-softmmu. qemu-system-armel is also  what the top-level 'emulator'
# launcher expects.
QEMU2_TARGET_SYSTEM := $(QEMU2_TARGET_CPU)
ifeq (arm,$(QEMU2_TARGET))
    QEMU2_TARGET_SYSTEM := armel
endif

# If $(QEMU2_TARGET) is $1, then return $2, or the empty string otherwise.
qemu2-if-target = $(if $(filter $1,$(QEMU2_TARGET)),$2,$3)

# If $(QEMU2_TARGET_CPU) is $1, then return $2, or the empty string otherwise.
qemu2-if-target-arch = $(if $(filter $1,$(QEMU2_TARGET_CPU)),$2)

# A library that contains all QEMU2 target-specific sources, excluding
# anything implemented by the glue code.

QEMU2_SYSTEM_CFLAGS := \
    $(QEMU2_CFLAGS) \
    -DNEED_CPU_H \

QEMU2_SYSTEM_INCLUDES := \
    $(QEMU2_CONFIG_DIR) \
    $(QEMU2_INCLUDES) \
    $(QEMU2_DEPS_TOP_DIR)/include \
    $(call qemu2-if-linux,$(LOCAL_PATH)/linux-headers) \
    $(LOCAL_PATH)/android-qemu2-glue/config/target-$(QEMU2_TARGET) \
    $(LOCAL_PATH)/target/$(QEMU2_TARGET_TARGET) \
    $(LOCAL_PATH)/tcg \
    $(LOCAL_PATH)/tcg/i386 \
    $(LOCAL_PATH)/accel/tcg \

QEMU2_SYSTEM_LDFLAGS := $(QEMU2_DEPS_LDFLAGS)

QEMU2_SYSTEM_LDLIBS := \
    $(QEMU2_GLIB_LDLIBS) \
    $(QEMU2_PIXMAN_LDLIBS) \
    $(CXX_STD_LIB) \
    -lfdt \
    $(call qemu2-if-windows, -lvfw32 -ldxguid) \
    $(call qemu2-if-linux, -lpulse) \
    $(call qemu2-if-darwin, -weak_framework Hypervisor) \

QEMU2_SYSTEM_STATIC_LIBRARIES := \
    emulator-zlib \
    $(call qemu2-if-windows-msvc,,emulator-virglrenderer) \
    emulator-tinyepoxy \
    $(LIBUSB_STATIC_LIBRARIES) \
    $(call qemu2-if-linux, emulator-libbluez) \

$(call start-emulator-program,qemu-system-$(QEMU2_TARGET_SYSTEM))

LOCAL_WHOLE_STATIC_LIBRARIES += \
    libqemu2-system-$(QEMU2_TARGET_SYSTEM) \
    libqemu2-common \

LOCAL_STATIC_LIBRARIES += \
    $(QEMU2_SYSTEM_STATIC_LIBRARIES) \
    $(QEMU2_GLUE_STATIC_LIBRARIES) \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    libqemu2-util \


LOCAL_CFLAGS += \
    $(QEMU2_SYSTEM_CFLAGS) \
    -DCONFIG_ANDROID \

LOCAL_C_INCLUDES += \
    $(QEMU2_SYSTEM_INCLUDES) \
    $(QEMU2_GLUE_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \

# For now, use stubs/sdl-null.c as an empty/fake SDL UI backend.
# TODO: Use the glue code to use the Qt-based UI instead.
LOCAL_SRC_FILES += \
    android-qemu2-glue/main.cpp \
    $(call qemu2-if-target,x86 x86_64, \
        hw/i386/acpi-build.c \
        hw/i386/pc_piix.c \
        ) \
    $(call qemu2-if-windows, \
        android-qemu2-glue/stubs/win32-stubs.c \
        ) \
    vl.c \

LOCAL_LDFLAGS += $(QEMU2_SYSTEM_LDFLAGS) $(QEMU2_GLUE_LDFLAGS)
LOCAL_LDLIBS += $(QEMU2_SYSTEM_LDLIBS) $(QEMU2_GLUE_LDLIBS)

LOCAL_LDFLAGS += \
    $(QEMU2_DEPS_LDFLAGS) \

#Link tcmalloc_minimal for Linux only
ifeq ($(BUILD_TARGET_OS), linux)
    LOCAL_LDFLAGS += -L$(TCMALLOC_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib64
    LOCAL_LDLIBS += -ltcmalloc_minimal
endif

LOCAL_LDLIBS += \
    $(QEMU2_GLIB_LDLIBS) \
    $(QEMU2_PIXMAN_LDLIBS) \
    $(CXX_STD_LIB) \
    -lfdt \
    $(call qemu2-if-windows, -lvfw32 -ldxguid) \
    $(call qemu2-if-linux, -lpulse) \
    $(ANDROID_EMU_LDLIBS) \

LOCAL_INSTALL_DIR := qemu/$(BUILD_TARGET_TAG)

$(call end-emulator-program)
