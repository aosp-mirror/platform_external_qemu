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
    $(QEMU2_INCLUDES) \
    $(QEMU2_DEPS_TOP_DIR)/include \
    $(call qemu2-if-linux,$(LOCAL_PATH)/linux-headers) \
    $(LOCAL_PATH)/android-qemu2-glue/config/target-$(QEMU2_TARGET) \
    $(LOCAL_PATH)/target-$(QEMU2_TARGET_TARGET) \
    $(LOCAL_PATH)/tcg \
    $(LOCAL_PATH)/tcg/i386 \

QEMU2_SYSTEM_LDFLAGS := $(QEMU2_DEPS_LDFLAGS)

QEMU2_SYSTEM_LDLIBS := \
    $(QEMU2_GLIB_LDLIBS) \
    $(QEMU2_PIXMAN_LDLIBS) \
    $(CXX_STD_LIB) \
    -lfdt \
    $(call qemu2-if-windows, -lvfw32 -ldxguid) \
    $(call qemu2-if-linux, -lpulse) \

QEMU2_SYSTEM_STATIC_LIBRARIES := \
    emulator-zlib

$(call start-emulator-library,libqemu2-system-$(QEMU2_TARGET_SYSTEM))

LOCAL_CFLAGS += \
    $(QEMU2_SYSTEM_CFLAGS) \
    -DPOISON_CONFIG_ANDROID \

LOCAL_C_INCLUDES += \
    $(QEMU2_SYSTEM_INCLUDES) \
    $(call qemu2-if-target,arm arm64,$(LOCAL_PATH)/disas/libvixl) \

LOCAL_SRC_FILES += \
    $(QEMU2_TARGET_SOURCES) \
    $(QEMU2_TARGET_$(QEMU2_TARGET_CPU)_SOURCES) \
    $(QEMU2_TARGET_$(QEMU2_TARGET_CPU)_SOURCES_$(BUILD_TARGET_TAG))

ifeq (arm64,$(QEMU2_TARGET))
LOCAL_GENERATED_SOURCES += $(QEMU2_AUTO_GENERATED_DIR)/gdbstub-xml-arm64.c
else ifeq (arm,$(QEMU2_TARGET))
LOCAL_GENERATED_SOURCES += $(QEMU2_AUTO_GENERATED_DIR)/gdbstub-xml-arm.c
else
LOCAL_SRC_FILES += stubs/gdbstub.c
endif

LOCAL_SRC_FILES += \
    stubs/target-get-monitor-def.c \
    $(call qemu2-if-target,arm arm64 mips mips64,\
        stubs/qmp_pc_dimm_device_list.c \
        ) \
    $(call qemu2-if-target,x86 x86_64,, \
        stubs/pc_madt_cpu_entry.c \
        stubs/smbios_type_38.c \
        stubs/target-monitor-defs.c \
        ) \
    $(call qemu2-if-target,mips mips64, \
        stubs/dump.c \
        stubs/arch-query-cpu-def.c \
        ) \
    $(call qemu2-if-linux,, \
        stubs/vhost.c \
        ) \

LOCAL_SRC_FILES += \
    $(call qemu2-if-target,x86 x86_64, \
        $(call qemu2-if-linux, hax-stub.c), \
        hax-stub.c \
    ) \

LOCAL_PREBUILTS_OBJ_FILES += \
    $(call qemu2-if-windows,$(QEMU2_AUTO_GENERATED_DIR)/version.o)

$(call end-emulator-library)

# The upstream version of QEMU2, without any Android-specific hacks.
# This uses the regular SDL2 UI backend.

$(call start-emulator-program,qemu-upstream-$(QEMU2_TARGET_SYSTEM))

LOCAL_WHOLE_STATIC_LIBRARIES += \
    libqemu2-system-$(QEMU2_TARGET_SYSTEM) \
    libqemu2-common \

LOCAL_STATIC_LIBRARIES += \
    $(QEMU2_SYSTEM_STATIC_LIBRARIES) \

LOCAL_CFLAGS += \
    $(QEMU2_SYSTEM_CFLAGS) \

LOCAL_C_INCLUDES += \
    $(QEMU2_SYSTEM_INCLUDES) \
    $(QEMU2_SDL2_INCLUDES) \

LOCAL_SRC_FILES += \
    $(call qemu2-if-target,x86 x86_64, \
        hw/i386/acpi-build.c \
        hw/i386/pc_piix.c \
        ) \
    $(call qemu2-if-windows, \
        stubs/win32-stubs.c \
        ) \
    ui/sdl2.c \
    ui/sdl2-input.c \
    ui/sdl2-2d.c \
    vl.c \

LOCAL_LDFLAGS += $(QEMU2_SYSTEM_LDFLAGS)

LOCAL_LDLIBS += \
    $(QEMU2_SYSTEM_LDLIBS) \
    $(QEMU2_SDL2_LDLIBS) \

LOCAL_INSTALL_DIR := qemu/$(BUILD_TARGET_TAG)

$(call end-emulator-program)


# The emulator-specific version of QEMU2, with CONFIG_ANDROID defined at
# compile-time.

$(call start-emulator-program,qemu-system-$(QEMU2_TARGET_SYSTEM))

LOCAL_WHOLE_STATIC_LIBRARIES += \
    libqemu2-glue \
    libqemu2-system-$(QEMU2_TARGET_SYSTEM) \
    libqemu2-common \

LOCAL_STATIC_LIBRARIES += \
    $(QEMU2_SYSTEM_STATIC_LIBRARIES) \
    $(QEMU2_GLUE_STATIC_LIBRARIES) \
    $(ANDROID_EMU_STATIC_LIBRARIES) \

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
