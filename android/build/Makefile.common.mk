BUILD_TARGET_TAG := $(BUILD_TARGET_OS)-$(BUILD_TARGET_ARCH)

# Build libext4_utils and related modules/
include $(LOCAL_PATH)/android/third_party/zlib.mk
include $(LOCAL_PATH)/android/third_party/libcurl.mk
include $(LOCAL_PATH)/android/third_party/libuuid.mk
include $(LOCAL_PATH)/android/third_party/libxml2.mk
include $(LOCAL_PATH)/android/third_party/mman-win32/libmman-win32.mk
include $(LOCAL_PATH)/android/third_party/libsparse/sources.mk
include $(LOCAL_PATH)/android/third_party/libselinux/sources.mk
include $(LOCAL_PATH)/android/third_party/libwebp/sources.mk
include $(LOCAL_PATH)/android/third_party/ext4_utils/sources.mk
include $(LOCAL_PATH)/android/third_party/libbreakpad.mk
include $(LOCAL_PATH)/android/third_party/Qt5.mk
include $(LOCAL_PATH)/android/third_party/jpeg-6b/libjpeg.mk
include $(LOCAL_PATH)/android/third_party/libpng.mk
include $(LOCAL_PATH)/android/third_party/mini-glib/sources.make
include $(LOCAL_PATH)/android/third_party/googletest/Android.mk
include $(LOCAL_PATH)/android/third_party/libANGLEtranslation.mk
include $(LOCAL_PATH)/android/third_party/Protobuf.mk
include $(LOCAL_PATH)/android/third_party/liblz4.mk
include $(LOCAL_PATH)/android/third_party/libffmpeg.mk
include $(LOCAL_PATH)/android/third_party/libx264.mk
include $(LOCAL_PATH)/android/third_party/libvpx.mk
include $(LOCAL_PATH)/android/third_party/libsdl2.mk

ifeq (true,$(BUILD_BENCHMARKS))
include $(LOCAL_PATH)/android/third_party/regex-win32/sources.mk
include $(LOCAL_PATH)/android/third_party/google-benchmark/sources.mk
endif

EMULATOR_VERSION_CFLAGS :=

ANDROID_SDK_TOOLS_REVISION := $(strip $(ANDROID_SDK_TOOLS_REVISION))
ifdef ANDROID_SDK_TOOLS_REVISION
    EMULATOR_VERSION_CFLAGS += -DANDROID_SDK_TOOLS_REVISION=$(ANDROID_SDK_TOOLS_REVISION)
endif

ANDROID_SDK_TOOLS_BUILD_NUMBER := $(strip $(ANDROID_SDK_TOOLS_BUILD_NUMBER))
ifdef ANDROID_SDK_TOOLS_BUILD_NUMBER
    EMULATOR_VERSION_CFLAGS += -DANDROID_SDK_TOOLS_BUILD_NUMBER=$(ANDROID_SDK_TOOLS_BUILD_NUMBER)
endif

EMULATOR_COMMON_CFLAGS := -Werror=implicit-function-declaration

# Include the emulator version definition from Makefile.common.mk
EMULATOR_COMMON_CFLAGS += $(EMULATOR_VERSION_CFLAGS)

ifeq (true,$(BUILD_DEBUG))
    EMULATOR_COMMON_CFLAGS += -DENABLE_DLOG=1
else
    EMULATOR_COMMON_CFLAGS += -DENABLE_DLOG=0
endif

EMULATOR_CRASHUPLOAD := $(strip $(EMULATOR_CRASHUPLOAD))
ifdef EMULATOR_CRASHUPLOAD
    EMULATOR_COMMON_CFLAGS += -DCRASHUPLOAD=$(EMULATOR_CRASHUPLOAD)
endif

ifeq (true,$(BUILD_SNAPSHOT_PROFILE))
    EMULATOR_COMMON_CFLAGS += -DSNAPSHOT_PROFILE
endif

# $(BUILD_OBJS_DIR)/build is required to access config-host.h
# $(generated-proto-sources-dir) is needed to access generated
# protobuff headers.
EMULATOR_COMMON_INCLUDES := \
    $(BUILD_OBJS_DIR)/build \
    $(generated-proto-sources-dir) \
    $(LIBMMAN_WIN32_INCLUDES) \

EMUGL_SRCDIR := $(LOCAL_PATH)/android/android-emugl
EMUGL_INCLUDES := $(EMUGL_SRCDIR)/host/include
ifeq (true,$(BUILD_EMUGL_PRINTOUT))
    EMUGL_USER_CFLAGS := -DOPENGL_DEBUG_PRINTOUT
else
    EMUGL_USER_CFLAGS :=
endif

##############################################################################
##############################################################################
###
###  gen-hw-config-defs: Generate hardware configuration definitions header
###
###  The 'gen-hw-config.py' script is used to generate the hw-config-defs.h
###  header from the an .ini file like android/avd/hardware-properties.ini
###
###  Due to the way the Android build system works, we need to regenerate
###  it for each module (the output will go into a module-specific directory).
###
###  This defines a function that can be used inside a module definition
###
###  $(call gen-hw-config-defs)
###

# First, define a rule to generate a dummy "emulator_hw_config_defs" module
# which purpose is simply to host the generated header in its output directory.
intermediates := $(call intermediates-dir-for,$(BUILD_TARGET_BITS),emulator_hw_config_defs)

QEMU_HARDWARE_PROPERTIES_INI := \
    $(_BUILD_ROOT)/android/android-emu/android/avd/hardware-properties.ini

QEMU_HW_CONFIG_DEFS_H := $(intermediates)/android/avd/hw-config-defs.h
$(QEMU_HW_CONFIG_DEFS_H): PRIVATE_PATH := $(_BUILD_ROOT)/android/scripts
$(QEMU_HW_CONFIG_DEFS_H): PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/gen-hw-config.py $< $@
$(QEMU_HW_CONFIG_DEFS_H): $(QEMU_HARDWARE_PROPERTIES_INI) $(PRIVATE_CUSTOM_TOOL)
	$(hide) rm -f $@
	$(transform-generated-source)

QEMU_HW_CONFIG_DEFS_INCLUDES := $(intermediates)

# Second, define a function that needs to be called inside each module that contains
# a source file that includes the generated header file.
gen-hw-config-defs = \
  $(eval LOCAL_GENERATED_SOURCES += $(QEMU_HW_CONFIG_DEFS_H))\
  $(eval LOCAL_C_INCLUDES += $(QEMU_HW_CONFIG_DEFS_INCLUDES))

ifeq ($(BUILD_TARGET_OS),windows)
  # on Windows, link the icon file as well into the executable
  # unfortunately, our build system doesn't help us much, so we need
  # to use some weird pathnames to make this work...

WINDRES_CPU_32 := i386
WINDRES_CPU_64 := x86-64

EMULATOR_ICON_OBJ := $(BUILD_OBJS_DIR)/build/emulator_icon$(BUILD_TARGET_BITS).o
$(EMULATOR_ICON_OBJ): PRIVATE_TARGET := $(WINDRES_CPU_$(BUILD_TARGET_BITS))
$(EMULATOR_ICON_OBJ): $(LOCAL_PATH)/android/images/emulator_icon.rc
	@echo "Windres ($(PRIVATE_TARGET)): $@"
	$(hide) $(BUILD_TARGET_WINDRES) --target=pe-$(PRIVATE_TARGET) $< -I $(LOCAL_PATH)/android/images -o $@

# Usage: $(eval $(call insert-windows-icon))
define insert-windows-icon
    LOCAL_PREBUILT_OBJ_FILES += $(EMULATOR_ICON_OBJ)
endef

endif  # BUILD_TARGET_OS == windows

include $(LOCAL_PATH)/android/android-emu/Makefile.android-emu.mk
include $(LOCAL_PATH)/android/android-emu/Makefile.crash-service.mk

include $(LOCAL_PATH)/android/qemu1/Makefile.qemu1-common.mk

# We want to build all variants of the emulator binaries. This makes
# it easier to catch target-specific regressions during emulator development.
EMULATOR_TARGET_ARCH := arm
include $(LOCAL_PATH)/android/qemu1/Makefile.qemu1-target.mk

# Note: the same binary handles x86 and x86_64
EMULATOR_TARGET_ARCH := x86
include $(LOCAL_PATH)/android/qemu1/Makefile.qemu1-target.mk

EMULATOR_TARGET_ARCH := mips
include $(LOCAL_PATH)/android/qemu1/Makefile.qemu1-target.mk

##############################################################################
##############################################################################
###
###  emulator: LAUNCHER FOR TARGET-SPECIFIC EMULATOR
###

# NOTE: Build as 32-bit or 64-bit executable, depending on the value of
#       EMULATOR_PROGRAM_BITNESS.
ifeq ($(BUILD_TARGET_BITS),$(EMULATOR_PROGRAM_BITNESS))
    $(call start-emulator-program, emulator)

    LOCAL_SRC_FILES := \
        android/emulator/main-emulator.cpp \

    # Needed to compile the call to androidQtSetupEnv() in main-emulator.cpp
    LOCAL_CFLAGS += -DCONFIG_QT

    LOCAL_C_INCLUDES += $(ANDROID_EMU_INCLUDES)

    # Need the build number as well
    LOCAL_CFLAGS += $(EMULATOR_VERSION_CFLAGS)

    LOCAL_STATIC_LIBRARIES := $(ANDROID_EMU_STATIC_LIBRARIES)

    LOCAL_LDLIBS += $(ANDROID_EMU_LDLIBS)

    # Ensure this is always built, even if 32-bit binaries are disabled.
    LOCAL_IGNORE_BITNESS := true

    ifeq ($(BUILD_TARGET_OS),windows)
    $(eval $(call insert-windows-icon))
    endif

    # To avoid runtime linking issues on Linux and Windows, a custom copy of the
    # C++ standard library is copied to $(BUILD_OBJS_DIR)/lib[64], a path that is added
    # to the runtime library search path by the top-level 'emulator' launcher
    # program before it spawns the emulation engine. However, 'emulator' cannot
    # use these versions of the library, so statically link it against the
    # executable instead.
    $(call local-link-static-c++lib)

    $(call end-emulator-program)

    ##############################################################################
    ###
    ###  emulator-check: a standalone question answering service
    ###

    $(call start-emulator-program, emulator-check)

    LOCAL_SRC_FILES := \
        android/emulator-check/main-emulator-check.cpp \
        android/emulator-check/PlatformInfo.cpp \

    LOCAL_C_INCLUDES += $(ANDROID_EMU_INCLUDES)
    LOCAL_STATIC_LIBRARIES := $(ANDROID_EMU_STATIC_LIBRARIES)
    LOCAL_LDLIBS := $(ANDROID_EMU_LDLIBS)

    LOCAL_IGNORE_BITNESS := true

    ifeq ($(BUILD_TARGET_OS),windows)
        $(eval $(call insert-windows-icon))
    endif

    ifeq ($(BUILD_TARGET_OS),linux)
        # For PlatformInfo.cpp
        LOCAL_LDLIBS += -lX11
    endif

    $(call local-link-static-c++lib)

    $(call end-emulator-program)
endif  # BUILD_TARGET_BITS == EMULATOR_PROGRAM_BITNESS

##############################################################################
##############################################################################
###
###  GPU emulation libraries
###
include $(EMUGL_SRCDIR)/Android.mk

##############################################################################
##############################################################################
###
###  QEMU2
###
ifdef QEMU2_TOP_DIR
include $(QEMU2_TOP_DIR)/android-qemu2-glue/build/Makefile.qemu2.mk
endif
