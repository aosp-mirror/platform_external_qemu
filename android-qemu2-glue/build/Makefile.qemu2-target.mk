# Rules to build the final target-specific QEMU2 binaries from sources.

QEMU2_TARGET_CPU_x86 := i386
QEMU2_TARGET_CPU_x86_64 := x86_64
QEMU2_TARGET_CPU_mips64 := mips64el
QEMU2_TARGET_CPU_arm64 := aarch64

QEMU2_TARGET_TARGET_x86 := i386
QEMU2_TARGET_TARGET_x86_64 := i386
QEMU2_TARGET_TARGET_mips64 := mips
QEMU2_TARGET_TARGET_arm64 := arm

QEMU2_TARGET_CPU := $(QEMU2_TARGET_CPU_$(QEMU2_TARGET))
QEMU2_TARGET_TARGET := $(QEMU2_TARGET_TARGET_$(QEMU2_TARGET))

# If $(QEMU2_TARGET) is $1, then return $2, or the empty string otherwise.
qemu2-if-target = $(if $(filter $1,$(QEMU2_TARGET)),$2,$3)

# If $(QEMU2_TARGET_CPU) is $1, then return $2, or the empty string otherwise.
qemu2-if-target-arch = $(if $(filter $1,$(QEMU2_TARGET_CPU)),$2)

$(call start-emulator-program,qemu-system-$(QEMU2_TARGET_CPU))

LOCAL_CPP_EXTENSION := .cc

LOCAL_CFLAGS += $(QEMU2_CFLAGS)

LOCAL_C_INCLUDES += \
    $(QEMU2_INCLUDES) \
    $(QEMU2_DEPS_TOP_DIR)/include \
    $(call qemu2-if-linux,$(LOCAL_PATH)/linux-headers) \
    $(LOCAL_PATH)/android-qemu2-glue/config/target-$(QEMU2_TARGET) \
    $(LOCAL_PATH)/target-$(QEMU2_TARGET_TARGET) \
    $(LOCAL_PATH)/tcg \
    $(LOCAL_PATH)/tcg/i386 \
    $(LOCAL_PATH)/target-$(QEMU2_TARGET_TARGET) \

LOCAL_CFLAGS += -DNEED_CPU_H

LOCAL_SRC_FILES += \
    $(QEMU2_TARGET_SOURCES) \
    $(QEMU2_TARGET_$(QEMU2_TARGET_CPU)_SOURCES) \
    $(QEMU2_TARGET_$(QEMU2_TARGET_CPU)_SOURCES_$(HOST_OS)-$(HOST_ARCH))

ifeq (arm64,$(QEMU2_TARGET))
LOCAL_GENERATED_SOURCES += $(QEMU2_AUTO_GENERATED_DIR)/gdbstub-xml-arm64.c
else
LOCAL_SRC_FILES += stubs/gdbstub.c
endif

LOCAL_SRC_FILES += \
    $(call qemu2-if-target,arm arm64 mips mips64,\
        stubs/dump.c \
        stubs/pci-drive-hot-add.c \
        stubs/qmp_pc_dimm_device_list.c \
        ) \
    $(call qemu2-if-target,mips mips64, \
        stubs/arch-query-cpu-def.c)

LOCAL_PREBUILTS_OBJ_FILES += \
    $(call qemu2-if-windows,$(QEMU2_AUTO_GENERATED_DIR)/version.o)

LOCAL_WHOLE_STATIC_LIBRARIES += \
    libqemu2_common \
    $(call qemu2-if-target,arm64, libqemu2_common_aarch64) \

LOCAL_STATIC_LIBRARIES += $(SDL2_STATIC_LIBRARIES)

LOCAL_LDFLAGS += $(QEMU2_DEPS_LDFLAGS)

LOCAL_LDLIBS += \
    $(QEMU2_GLIB_LDLIBS) \
    $(QEMU2_PIXMAN_LDLIBS) \
    $(SDL2_LDLIBS) \
    $(CXX_STD_LIB) \
    -lfdt \
    -lz \
    $(call qemu2-if-linux, -lpulse) \

LOCAL_INSTALL_DIR := qemu/$(HOST_OS)-$(HOST_ARCH)

$(call end-emulator-program)
