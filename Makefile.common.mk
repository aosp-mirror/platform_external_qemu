# Build libext4_utils and related modules/
include $(LOCAL_PATH)/distrib/zlib.mk
include $(LOCAL_PATH)/distrib/libcurl.mk
include $(LOCAL_PATH)/distrib/libxml2.mk
include $(LOCAL_PATH)/distrib/libsparse/sources.mk
include $(LOCAL_PATH)/distrib/libselinux/sources.mk
include $(LOCAL_PATH)/distrib/ext4_utils/sources.mk
include $(LOCAL_PATH)/distrib/libbreakpad_client.mk
include $(LOCAL_PATH)/distrib/Qt5.mk

include $(LOCAL_PATH)/Makefile.qemu1-common.mk

ifeq ($(HOST_OS),windows)
  # on Windows, link the icon file as well into the executable
  # unfortunately, our build system doesn't help us much, so we need
  # to use some weird pathnames to make this work...

WINDRES_CPU_32 := i386
WINDRES_CPU_64 := x86-64

EMULATOR_ICON_OBJ := $(OBJS_DIR)/build/emulator_icon$(HOST_BITS).o
$(EMULATOR_ICON_OBJ): PRIVATE_TARGET := $(WINDRES_CPU_$(HOST_BITS))
$(EMULATOR_ICON_OBJ): $(LOCAL_PATH)/images/emulator_icon.rc
	@echo "Windres ($(PRIVATE_TARGET)): $@"
	$(hide) $(MY_WINDRES) --target=pe-$(PRIVATE_TARGET) $< -I $(LOCAL_PATH)/images -o $@

# Usage: $(eval $(call insert-windows-icon))
define insert-windows-icon
    LOCAL_PREBUILT_OBJ_FILES += $(EMULATOR_ICON_OBJ)
endef

endif  # HOST_OS == windows

# We want to build all variants of the emulator binaries. This makes
# it easier to catch target-specific regressions during emulator development.
EMULATOR_TARGET_ARCH := arm
include $(LOCAL_PATH)/Makefile.qemu1-target.mk

# Note: the same binary handles x86 and x86_64
EMULATOR_TARGET_ARCH := x86
include $(LOCAL_PATH)/Makefile.qemu1-target.mk
include $(LOCAL_PATH)/Makefile.qemu-launcher.mk

EMULATOR_TARGET_ARCH := x86_64
include $(LOCAL_PATH)/Makefile.qemu-launcher.mk

EMULATOR_TARGET_ARCH := mips
include $(LOCAL_PATH)/Makefile.qemu1-target.mk

EMULATOR_TARGET_ARCH := mips64
include $(LOCAL_PATH)/Makefile.qemu-launcher.mk

EMULATOR_TARGET_ARCH := arm64
include $(LOCAL_PATH)/Makefile.qemu-launcher.mk

##############################################################################
##############################################################################
###
###  emulator: LAUNCHER FOR TARGET-SPECIFIC EMULATOR
###

# NOTE: Build as 32-bit or 64-bit executable, depending on the value of
#       EMULATOR_PROGRAM_BITNESS.
ifeq ($(HOST_BITS),$(EMULATOR_PROGRAM_BITNESS))
$(call start-emulator-program, emulator)
LOCAL_SRC_FILES := \
    android/main-emulator.c \

ifdef EMULATOR_USE_QT
    LOCAL_SRC_FILES += \
        android/qt/qt_path.cpp \
        android/qt/qt_setup.cpp \

    # Needed to compile the call to androidQtSetupEnv() in main-emulator.c
    LOCAL_CFLAGS += -DCONFIG_QT
endif

LOCAL_STATIC_LIBRARIES := emulator-common
# Ensure this is always built, even if 32-bit binaries are disabled.
LOCAL_IGNORE_BITNESS := true

LOCAL_LDLIBS += $(CXX_STD_LIB)
LOCAL_GENERATE_SYMBOLS := true

ifeq ($(HOST_OS),windows)
$(eval $(call insert-windows-icon))
endif

# This launcher program sets up certain library search paths so that the
# rexec'ed target specific emulator can load those shared libraries. OTOH, this
# program itself does not have that luxury. So, link in everything statically.
LOCAL_CFLAGS += $(BUILD_STATIC_FLAGS)
LOCAL_LDFLAGS += $(BUILD_STATIC_FLAGS)

$(call end-emulator-program)
endif  # HOST_BITS == EMULATOR_PROGRAM_BITNESS

include $(LOCAL_PATH)/Makefile.tests.mk

##############################################################################
##############################################################################
###
###  GPU emulation libraries
###
###  Build directly from sources when using the standalone build.
###
ifeq (,$(strip $(wildcard $(EMULATOR_EMUGL_SOURCES_DIR))))
$(error Cannot find GPU emulation sources directory: $(EMULATOR_EMUGL_SOURCES_DIR))
endif

ifeq (true,$(BUILD_DEBUG_EMULATOR))
EMUGL_BUILD_DEBUG := 1
endif
include $(EMULATOR_EMUGL_SOURCES_DIR)/Android.mk

ifdef QEMU2_TOP_DIR
include $(QEMU2_TOP_DIR)/android-qemu2-glue/build/Makefile.qemu2.mk
endif
