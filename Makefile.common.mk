# Build libext4_utils and related modules/
include $(LOCAL_PATH)/distrib/zlib.mk
include $(LOCAL_PATH)/distrib/libsparse/sources.mk
include $(LOCAL_PATH)/distrib/libselinux/sources.mk
include $(LOCAL_PATH)/distrib/ext4_utils/sources.mk

include $(LOCAL_PATH)/Makefile.qemu1-common.mk

ifeq ($(HOST_OS),windows)
  # on Windows, link the icon file as well into the executable
  # unfortunately, our build system doesn't help us much, so we need
  # to use some weird pathnames to make this work...

  # Usage: $(eval $(call insert-windows-icon))
  define insert-windows-icon
    LOCAL_PREBUILT_OBJ_FILES += images/emulator_icon$(LOCAL_MODULE_BITS).o
  endef

# This seems to be the only way to add an object file that was not generated from
# a C/C++/Java source file to our build system. and very unfortunately,
# $(TOPDIR)/$(LOCALPATH) will always be prepended to this value, which forces
# us to put the object file in the source directory.
$(LOCAL_PATH)/images/emulator_icon32.o: $(LOCAL_PATH)/images/android_icon.rc
	@echo "Windres (x86): $@"
	$(MY_WINDRES) --target=pe-i386 $< -I $(LOCAL_PATH)/images -o $@

$(LOCAL_PATH)/images/emulator_icon64.o: $(LOCAL_PATH)/images/android_icon.rc
	@echo "Windres (x86_64): $@"
	$(MY_WINDRES) --target=pe-x86-64 $< -I $(LOCAL_PATH)/images -o $@
endif

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
ifeq (32,$(EMULATOR_PROGRAM_BITNESS))
$(call start-emulator-program, emulator)
else
$(call start-emulator64-program, emulator)
endif
LOCAL_SRC_FILES := \
    android/main-emulator.c \

ifdef EMULATOR_USE_QT
    LOCAL_SRC_FILES += \
        android/qt/qt_path.cpp \
        android/qt/qt_setup.cpp \

    # Needed to compile the call to androidQtSetupEnv() in main-emulator.c
    LOCAL_CFLAGS += -DCONFIG_QT
endif

ifeq (32,$(EMULATOR_PROGRAM_BITNESS))
    LOCAL_STATIC_LIBRARIES := emulator-common
    # Ensure this is always built, even if 32-bit binaries are disabled.
    LOCAL_IGNORE_BITNESS := true
else
    LOCAL_STATIC_LIBRARIES := emulator64-common
endif

LOCAL_LDLIBS += $(CXX_STD_LIB)
LOCAL_GENERATE_SYMBOLS := true

ifeq ($(HOST_OS),windows)
$(eval $(call insert-windows-icon))
endif

$(call end-emulator-program)

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
