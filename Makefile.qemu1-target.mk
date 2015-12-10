# This file is included several times to build target-specific
# modules for the Android emulator. It will be called several times
# for arm, x86 and mips
#

ifndef EMULATOR_TARGET_ARCH
$(error EMULATOR_TARGET_ARCH is not defined!)
endif

EMULATOR_TARGET_CPU := $(EMULATOR_TARGET_ARCH)
ifeq ($(EMULATOR_TARGET_CPU),x86)
  EMULATOR_TARGET_CPU := i386
endif

##############################################################################
##############################################################################
###
###  emulator-target-$CPU: target-specific emulation code.
###
###  Used by both the core and standalone programs.
###

TCG_TARGET := $(HOST_ARCH)
ifeq ($(HOST_ARCH),x86)
  TCG_TARGET := i386
endif
ifeq ($(HOST_ARCH),x86_64)
  TCG_TARGET := i386
endif

# Common compiler flags for all target-dependent libraries
EMULATOR_TARGET_INCLUDES := \
    $(LOCAL_PATH)/android/config/target-$(EMULATOR_TARGET_ARCH) \
    $(LOCAL_PATH)/target-$(EMULATOR_TARGET_CPU) \
    $(LOCAL_PATH)/fpu \
    $(LOCAL_PATH)/tcg \
    $(LOCAL_PATH)/tcg/$(TCG_TARGET) \
    $(LOCAL_PATH)/slirp-android \
    $(LIBCURL_INCLUDES) \

EMULATOR_TARGET_CFLAGS := \
    -DNEED_CPU_H \
    -DTARGET_ARCH=\"$(EMULATOR_TARGET_ARCH)\"

## one for 32-bit
$(call start-emulator-library, emulator-target-$(EMULATOR_TARGET_CPU))
LOCAL_CFLAGS := \
    $(QEMU1_COMMON_CFLAGS) \
    $(EMULATOR_TARGET_CFLAGS) \

# These are required by the TCG engine.
LOCAL_CFLAGS += \
    -fno-PIC \
    -fomit-frame-pointer \
    -Wno-sign-compare

# required to ensure we properly initialize virtual audio hardware
LOCAL_CFLAGS += \
    -DHAS_AUDIO \

LOCAL_C_INCLUDES += \
    $(QEMU1_COMMON_INCLUDES) \
    $(EMULATOR_TARGET_INCLUDES) \

LOCAL_SRC_FILES += \
    android-qemu1-glue/main.c \
    android-qemu1-glue/qemu-battery-agent-impl.c \
    android-qemu1-glue/qemu-user-event-agent-impl.c \
    android-qemu1-glue/qemu-vm-operations-impl.c \
    arch_init.c \
    backends/msmouse.c \
    code-profile.c \
    cpus.c \
    cputlb.c \
    cpu-exec.c  \
    disas.c \
    dma-helpers.c \
    exec.c \
    fpu/softfloat.c \
    gdbstub.c \
    hw/android/goldfish/audio.c \
    hw/android/goldfish/battery.c \
    hw/android/goldfish/device.c \
    hw/android/goldfish/events_device.c \
    hw/android/goldfish/fb.c \
    hw/android/goldfish/mmc.c   \
    hw/android/goldfish/nand.c \
    hw/android/goldfish/pipe.c \
    hw/android/goldfish/profile.c \
    hw/android/goldfish/trace.c \
    hw/android/goldfish/tty.c \
    hw/android/goldfish/vmem.c \
    hw/core/dma.c \
    hw/core/irq.c \
    hw/core/loader.c \
    hw/core/qdev.c \
    hw/core/sysbus.c \
    hw/pci/pci.c \
    hw/watchdog/watchdog.c \
    log-rotate-android.c \
    main-loop.c \
    memory-android.c \
    monitor-android.c \
    qemu-timer.c \
    translate-all.c \
    tcg/optimize.c \
    tcg/tcg.c \
    tcg-runtime.c \
    ui/keymaps.c \
    util/bitmap.c \
    util/bitops.c \
    util/host-utils.c \
    util/qemu-timer-common.c \
    vl-android.c \

ifeq ($(EMULATOR_TARGET_ARCH),arm)
LOCAL_SRC_FILES += \
    disas/arm.c \
    hw/android/android_arm.c \
    hw/android/goldfish/interrupt.c \
    hw/android/goldfish/timer.c \
    hw/arm/armv7m.c \
    hw/arm/armv7m_nvic.c \
    hw/arm/boot.c \
    hw/arm/pic.c \
    hw/net/smc91c111.c \
    target-arm/arm-semi.c \
    target-arm/helper.c \
    target-arm/iwmmxt_helper.c \
    target-arm/machine.c \
    target-arm/op_helper.c \
    target-arm/neon_helper.c \
    target-arm/translate.c \

endif  # EMULATOR_TARGET_ARCH == arm

ifeq ($(EMULATOR_TARGET_ARCH),x86)
LOCAL_SRC_FILES += \
    hw/i386/pc.c \
    hw/i386/smbios.c \
    hw/input/pckbd.c \
    hw/input/ps2.c \
    hw/intc/apic.c \
    hw/intc/i8259.c \
    hw/intc/ioapic.c \
    hw/net/ne2000.c \
    hw/nvram/fw_cfg.c \
    hw/pci-host/piix.c \
    hw/timer/i8254.c \
    hw/timer/mc146818rtc.c \
    target-i386/cc_helper.c \
    target-i386/excp_helper.c \
    target-i386/fpu_helper.c \
    target-i386/helper.c \
    target-i386/int_helper.c \
    target-i386/machine.c \
    target-i386/mem_helper.c \
    target-i386/misc_helper.c \
    target-i386/seg_helper.c \
    target-i386/smm_helper.c \
    target-i386/svm_helper.c \
    target-i386/translate.c \

ifeq ($(HOST_OS),darwin)
LOCAL_SRC_FILES += \
    target-i386/hax-all.c \
    target-i386/hax-darwin.c \

endif

ifeq ($(HOST_OS),windows)
LOCAL_SRC_FILES += \
    target-i386/hax-all.c \
    target-i386/hax-windows.c \

endif

ifeq ($(HOST_OS),linux)
LOCAL_SRC_FILES += \
    target-i386/kvm.c \
    target-i386/kvm-gs-restore.c \
    kvm-all.c \
    kvm-android.c \

endif

endif  # EMULATOR_TARGET_ARCH == x86

ifeq ($(EMULATOR_TARGET_ARCH),mips)
LOCAL_SRC_FILES += \
    disas/mips.c \
    hw/android/android_mips.c \
    hw/android/goldfish/interrupt.c \
    hw/android/goldfish/timer.c \
    hw/mips/cputimer.c \
    hw/mips/mips_int.c \
    hw/mips/mips_pic.c \
    hw/net/smc91c111.c \
    target-mips/helper.c \
    target-mips/op_helper.c \
    target-mips/translate.c \
    target-mips/machine.c \

endif  # EMULATOR_TARGET_ARCH == mips

# What a mess, os-posix.c depends on the exact values of options
# which are target specific.
ifeq ($(HOST_OS),windows)
    LOCAL_SRC_FILES += \
        os-win32.c \
        tap-win32.c \
        util/oslib-win32.c \

else
    LOCAL_SRC_FILES += \
        os-posix.c \
        util/oslib-posix.c \

endif

$(call gen-hw-config-defs)
$(call gen-hx-header,qemu-options.hx,qemu-options.def,os-posix.c os-win32.c)
$(call end-emulator-library)

##############################################################################
##############################################################################
###
###  emulator-$ARCH: Standalone emulator program
###
###

$(call start-emulator-program, emulator$(HOST_SUFFIX)-$(EMULATOR_TARGET_ARCH))

LOCAL_STATIC_LIBRARIES += \
    emulator-libui \
    $(ANDROID_EMU_STATIC_LIBRARIES_QEMU1) \
    $(ANDROID_SKIN_STATIC_LIBRARIES) \
    $(EMULATOR_LIBUI_STATIC_LIBRARIES) \

LOCAL_WHOLE_STATIC_LIBRARIES := \
    emulator-libqemu \
    emulator-target-$(EMULATOR_TARGET_CPU) \

LOCAL_LDLIBS := \
    $(ANDROID_EMU_LDLIBS) \
    $(EMULATOR_LIBQEMU_LDLIBS) \
    $(EMULATOR_LIBUI_LDLIBS) \
    $(CXX_STD_LIB) \

LOCAL_LDFLAGS := \
    $(EMULATOR_LIBUI_LDFLAGS) \

LOCAL_GENERATE_SYMBOLS := true
$(call gen-hx-header,qemu-options.hx,qemu-options.def,vl-android.c qemu-options.h)
$(call gen-hw-config-defs)

ifeq ($(HOST_OS),windows)
  $(eval $(call insert-windows-icon))
endif

$(call end-emulator-program)
