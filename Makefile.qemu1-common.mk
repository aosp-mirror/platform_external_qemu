# When building this project, we actually generate several components which
# are the following:
#
#  - the emulator-ui program (which is target-agnostic)
#  - the target-specific qemu-android-$ARCH programs (headless emulation engines)
#  - the "standalone" emulator programs (embed both UI and engine in a single
#    binary and process), i.e. "emulator" for ARM and "emulator-x86" for x86.
#
# This file defines static host libraries that will be used by at least two
# of these components.
#

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
intermediates := $(call intermediates-dir-for,$(HOST_BITS),emulator_hw_config_defs)

QEMU_HARDWARE_PROPERTIES_INI := $(LOCAL_PATH)/android/avd/hardware-properties.ini
QEMU_HW_CONFIG_DEFS_H := $(intermediates)/android/avd/hw-config-defs.h
$(QEMU_HW_CONFIG_DEFS_H): PRIVATE_PATH := $(LOCAL_PATH)
$(QEMU_HW_CONFIG_DEFS_H): PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/android/tools/gen-hw-config.py $< $@
$(QEMU_HW_CONFIG_DEFS_H): $(QEMU_HARDWARE_PROPERTIES_INI) $(LOCAL_PATH)/android/tools/gen-hw-config.py
	$(hide) rm -f $@
	$(transform-generated-source)

QEMU_HW_CONFIG_DEFS_INCLUDES := $(intermediates)

# Second, define a function that needs to be called inside each module that contains
# a source file that includes the generated header file.
gen-hw-config-defs = \
  $(eval LOCAL_GENERATED_SOURCES += $(QEMU_HW_CONFIG_DEFS_H))\
  $(eval LOCAL_C_INCLUDES += $(QEMU_HW_CONFIG_DEFS_INCLUDES))

EMULATOR_USE_SDL2 := $(strip $(filter true,$(EMULATOR_USE_SDL2)))
EMULATOR_USE_QT := $(strip $(filter true,$(EMULATOR_USE_QT)))

##############################################################################
##############################################################################
###
###  emulator-common: LIBRARY OF COMMON FUNCTIONS
###
###  THESE ARE POTENTIALLY USED BY ALL COMPONENTS
###

common_LOCAL_CFLAGS =
common_LOCAL_SRC_FILES =

EMULATOR_COMMON_CFLAGS := -Werror=implicit-function-declaration

# Needed by everything about the host
# $(OBJS_DIR)/build contains config-host.h
# $(LOCAL_PATH)/include contains common headers.
EMULATOR_COMMON_CFLAGS += \
    -I$(OBJS_DIR)/build \
    -I$(LOCAL_PATH)/include

# Need to include "qapi-types.h" and other auto-generated files from
# android-configure.sh
EMULATOR_COMMON_CFLAGS += -I$(OBJS_DIR)/build/qemu1-qapi-auto-generated


ANDROID_SDK_TOOLS_REVISION := $(strip $(ANDROID_SDK_TOOLS_REVISION))
ifdef ANDROID_SDK_TOOLS_REVISION
    EMULATOR_COMMON_CFLAGS += -DANDROID_SDK_TOOLS_REVISION=$(ANDROID_SDK_TOOLS_REVISION)
endif

# Enable large-file support (i.e. make off_t a 64-bit value)
ifeq ($(HOST_OS),linux)
EMULATOR_COMMON_CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
endif

ifeq (true,$(BUILD_DEBUG_EMULATOR))
    EMULATOR_COMMON_CFLAGS += -DENABLE_DLOG=1
else
    EMULATOR_COMMON_CFLAGS += -DENABLE_DLOG=0
endif

###########################################################
# Zlib sources
#
EMULATOR_COMMON_CFLAGS += -I$(ZLIB_INCLUDES)

###########################################################
# GLib sources
#
GLIB_DIR := distrib/mini-glib
include $(LOCAL_PATH)/$(GLIB_DIR)/sources.make
EMULATOR_COMMON_CFLAGS += -I$(GLIB_INCLUDE_DIR)

common_LOCAL_SRC_FILES += $(GLIB_SOURCES)

EMULATOR_COMMON_CFLAGS += $(LIBCURL_CFLAGS)

###########################################################
# build the android-emu libraries
include $(LOCAL_PATH)/Makefile.android-emu.mk

###########################################################
# Android utility functions
#
common_LOCAL_SRC_FILES += \
    android/audio-test.c \
    android/core-init-utils.c \
    android/hw-kmsg.c \
    android/hw-lcd.c \
    android/multitouch-screen.c \
    android/multitouch-port.c \
    android/camera/camera-format-converters.c \
    android/camera/camera-service.c \
    android/utils/jpeg-compress.c \

common_LOCAL_CFLAGS += $(EMULATOR_COMMON_CFLAGS)

common_LOCAL_CFLAGS += $(LIBXML2_CFLAGS)
common_LOCAL_CFLAGS += -I$(LIBEXT4_UTILS_INCLUDES)

include $(LOCAL_PATH)/android/wear-agent/sources.mk

## one for 32-bit
$(call start-emulator-library, emulator-common)

LOCAL_CFLAGS += $(common_LOCAL_CFLAGS)

LOCAL_C_INCLUDES += \
    $(LIBCURL_INCLUDES) \
    $(LIBXML2_INCLUDES) \
    $(BREAKPAD_INCLUDES) \
    $(LIBJPEG_INCLUDES) \

LOCAL_SRC_FILES += $(common_LOCAL_SRC_FILES)

ifeq (32,$(EMULATOR_PROGRAM_BITNESS))
    LOCAL_IGNORE_BITNESS := true
endif
$(call gen-hw-config-defs)
$(call end-emulator-library)

##############################################################################
##############################################################################
###
###  emulator-libui: LIBRARY OF UI-RELATED FUNCTIONS
###
###  THESE ARE USED BY 'emulator-ui' AND THE STANDALONE PROGRAMS
###

common_LOCAL_CFLAGS =
common_LOCAL_SRC_FILES =
common_LOCAL_QT_MOC_SRC_FILES =
common_LOCAL_QT_RESOURCES =
common_LOCAL_CFLAGS += $(EMULATOR_COMMON_CFLAGS)

EMULATOR_LIBUI_CFLAGS :=
EMULATOR_LIBUI_INCLUDES :=
EMULATOR_LIBUI_LDLIBS :=
EMULATOR_LIBUI_LDFLAGS :=
EMULATOR_LIBUI_STATIC_LIBRARIES :=

###########################################################
# Libpng configuration
#
EMULATOR_LIBUI_INCLUDES += $(LIBPNG_INCLUDES)
EMULATOR_LIBUI_STATIC_LIBRARIES += emulator-libpng
common_LOCAL_SRC_FILES += android/loadpng.c

###########################################################
# Libjpeg configuration
#
EMULATOR_LIBUI_INCLUDES += $(LIBJPEG_INCLUDES)
EMULATOR_LIBUI_STATIC_LIBRARIES += emulator-libjpeg

##############################################################################
# SDL-related definitions
#

ifdef EMULATOR_USE_SDL2
    include $(LOCAL_PATH)/distrib/libsdl2.mk

    EMULATOR_LIBUI_CFLAGS += $(SDL2_CFLAGS)
    EMULATOR_LIBUI_INCLUDES += $(SDL2_INCLUDES)
    EMULATOR_LIBUI_LDLIBS += $(SDL2_LDLIBS)
    EMULATOR_LIBUI_STATIC_LIBRARIES += $(SDL2_STATIC_LIBRARIES)

    ifeq ($(HOST_OS),windows)
        # Special exception for Windows: -lmingw32 must appear before libSDLmain
        # on the link command-line, because it depends on _WinMain@16 which is
        # exported by the latter.
        EMULATOR_LIBUI_LDFLAGS += -lmingw32
        EMULATOR_LIBUI_CFLAGS += -Dmain=SDL_main
    else
        # The following is needed by SDL_LoadObject
        EMULATOR_LIBUI_LDLIBS += -ldl
    endif
endif  # EMULATOR_USE_SDL2

###########################################################################
# Qt-related definitions
#
ifdef EMULATOR_USE_QT
    EMULATOR_LIBUI_INCLUDES += $(QT_INCLUDES)
    EMULATOR_LIBUI_LDFLAGS += $(QT_LDFLAGS)
    EMULATOR_LIBUI_LDLIBS += $(QT_LDLIBS)
    EMULATOR_LIBUI_CFLAGS += $(LIBXML2_CFLAGS)
endif  # EMULATOR_USE_QT

# the skin support sources
#
include $(LOCAL_PATH)/android/skin/sources.mk
common_LOCAL_SRC_FILES += $(ANDROID_SKIN_SOURCES)

common_LOCAL_SRC_FILES += \
    android/gpu_frame.cpp \
    android/emulator-window.c \
    android/resource.c \
    android/user-config.c \

# enable MMX code for our skin scaler
ifeq ($(HOST_ARCH),x86)
common_LOCAL_CFLAGS += -DUSE_MMX=1 -mmmx
endif

common_LOCAL_CFLAGS += $(EMULATOR_LIBUI_CFLAGS)


ifeq ($(HOST_OS),windows)
# For capCreateCaptureWindow used in camera-capture-windows.c
EMULATOR_LIBUI_LDLIBS += -lvfw32
endif

## one for 32-bit
$(call start-emulator-library, emulator-libui)
LOCAL_CFLAGS += $(common_LOCAL_CFLAGS) $(ANDROID_SKIN_CFLAGS)
LOCAL_C_INCLUDES := $(EMULATOR_LIBUI_INCLUDES)
LOCAL_SRC_FILES += $(common_LOCAL_SRC_FILES)
LOCAL_QT_MOC_SRC_FILES := $(ANDROID_SKIN_QT_MOC_SRC_FILES)
LOCAL_QT_RESOURCES := $(ANDROID_SKIN_QT_RESOURCES)
LOCAL_QT_UI_SRC_FILES := $(ANDROID_SKIN_QT_UI_SRC_FILES)
$(call gen-hw-config-defs)
$(call end-emulator-library)

##############################################################################
##############################################################################
###
###  emulator-libqemu: TARGET-INDEPENDENT QEMU FUNCTIONS
###
###  THESE ARE USED BY EVERYTHING EXCEPT 'emulator-ui'
###

common_LOCAL_CFLAGS =
common_LOCAL_SRC_FILES =


EMULATOR_LIBQEMU_CFLAGS :=

AUDIO_SOURCES := noaudio.c wavaudio.c wavcapture.c mixeng.c
AUDIO_CFLAGS  := -I$(LOCAL_PATH)/audio -DHAS_AUDIO
AUDIO_LDLIBS  :=

ifeq ($(HOST_OS),darwin)
  CONFIG_COREAUDIO ?= yes
  AUDIO_CFLAGS += -DHOST_BSD=1
endif

ifeq ($(HOST_OS),windows)
  CONFIG_WINAUDIO ?= yes
endif

ifeq ($(HOST_OS),linux)
  CONFIG_OSS  ?= yes
  CONFIG_ALSA ?= yes
  CONFIG_PULSEAUDIO ?= yes
  CONFIG_ESD  ?= yes
endif

ifeq ($(HOST_OS),freebsd)
  CONFIG_OSS ?= yes
endif

ifeq ($(CONFIG_COREAUDIO),yes)
  AUDIO_SOURCES += coreaudio.c
  AUDIO_CFLAGS  += -DCONFIG_COREAUDIO
  AUDIO_LDLIBS  += -Wl,-framework,CoreAudio
endif

ifeq ($(CONFIG_WINAUDIO),yes)
  AUDIO_SOURCES += winaudio.c
  AUDIO_CFLAGS  += -DCONFIG_WINAUDIO
endif

ifeq ($(CONFIG_PULSEAUDIO),yes)
  AUDIO_SOURCES += paaudio.c audio_pt_int.c
  AUDIO_SOURCES += wrappers/pulse-audio.c
  AUDIO_CFLAGS  += -DCONFIG_PULSEAUDIO
endif

ifeq ($(CONFIG_ALSA),yes)
  AUDIO_SOURCES += alsaaudio.c audio_pt_int.c
  AUDIO_SOURCES += wrappers/alsa.c
  AUDIO_CFLAGS  += -DCONFIG_ALSA
endif

ifeq ($(CONFIG_ESD),yes)
  AUDIO_SOURCES += esdaudio.c
  AUDIO_SOURCES += wrappers/esound.c
  AUDIO_CFLAGS  += -DCONFIG_ESD
endif

ifeq ($(CONFIG_OSS),yes)
  AUDIO_SOURCES += ossaudio.c
  AUDIO_CFLAGS  += -DCONFIG_OSS
endif

AUDIO_SOURCES := $(call sort,$(AUDIO_SOURCES:%=audio/%))

# other flags
ifneq ($(HOST_OS),windows)
    AUDIO_LDLIBS += -ldl
else
endif


EMULATOR_LIBQEMU_CFLAGS += $(AUDIO_CFLAGS)
EMULATOR_LIBQEMU_LDLIBS += $(AUDIO_LDLIBS)

ifeq ($(QEMU_TARGET_XML_SOURCES),)
    QEMU_TARGET_XML_SOURCES := arm-core arm-neon arm-vfp arm-vfp3
    QEMU_TARGET_XML_SOURCES := $(QEMU_TARGET_XML_SOURCES:%=$(LOCAL_PATH)/gdb-xml/%.xml)
endif

$(call start-emulator-library, emulator-libqemu)

# gdbstub-xml.c contains C-compilable arrays corresponding to the content
# of $(LOCAL_PATH)/gdb-xml/, and is generated with the 'feature_to_c.sh' script.
#
intermediates = $(call local-intermediates-dir)
QEMU_GDBSTUB_XML_C = $(intermediates)/gdbstub-xml.c
$(QEMU_GDBSTUB_XML_C): PRIVATE_PATH := $(LOCAL_PATH)
$(QEMU_GDBSTUB_XML_C): PRIVATE_SOURCES := $(TARGET_XML_SOURCES)
$(QEMU_GDBSTUB_XML_C): PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/feature_to_c.sh $@ $(QEMU_TARGET_XML_SOURCES)
$(QEMU_GDBSTUB_XML_C): $(QEMU_TARGET_XML_SOURCES) $(LOCAL_PATH)/feature_to_c.sh
	$(hide) rm -f $@
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(QEMU_GDBSTUB_XML_C)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/slirp-android \
    $(intermediates) \
    $(LIBJPEG_INCLUDES) \

LOCAL_CFLAGS := \
    $(EMULATOR_COMMON_CFLAGS) \
    -W \
    -Wall \
    $(GCC_W_NO_MISSING_FIELD_INITIALIZERS) \
    -Wno-sign-compare \
    -fno-strict-aliasing \
    -Wno-unused-parameter \
    -D_XOPEN_SOURCE=600 \
    -D_BSD_SOURCE=1 \
    -DCONFIG_BDRV_WHITELIST=\"\" \
    $(EMULATOR_LIBQEMU_CFLAGS) \
    $(AUDIO_CFLAGS) \

# this is very important, otherwise the generated binaries may
# not link properly on our build servers
ifeq ($(HOST_OS),linux)
LOCAL_CFLAGS += -fno-stack-protector
endif

LOCAL_SRC_FILES += \
    $(AUDIO_SOURCES) \
    aio-android.c \
    android-qemu1-glue/android_qemud.cpp \
    android-qemu1-glue/base/async/Looper.cpp \
    android-qemu1-glue/base/files/QemuFileStream.cpp \
    android-qemu1-glue/display.c \
    android-qemu1-glue/emulation/charpipe.c \
    android-qemu1-glue/emulation/CharSerialLine.cpp \
    android-qemu1-glue/emulation/serial_line.cpp \
    android-qemu1-glue/looper-qemu.cpp \
    android-qemu1-glue/qemu-cellular-agent-impl.c \
    android-qemu1-glue/qemu-display-agent-impl.cpp \
    android-qemu1-glue/qemu-finger-agent-impl.c \
    android-qemu1-glue/qemu-location-agent-impl.c \
    android-qemu1-glue/qemu-net-agent-impl.c \
    android-qemu1-glue/qemu-sensors-agent-impl.c \
    android-qemu1-glue/qemu-setup.cpp \
    android-qemu1-glue/qemu-telephony-agent-impl.c \
    android-qemu1-glue/qemu-window-agent-impl.c \
    android-qemu1-glue/telephony/modem_init.cpp \
    android-qemu1-glue/utils/stream.cpp \
    async.c \
    block.c \
    blockdev.c \
    block/qcow2.c \
    block/qcow2-refcount.c \
    block/qcow2-snapshot.c \
    block/qcow2-cluster.c \
    block/raw.c \
    iohandler.c \
    ioport.c \
    migration-dummy-android.c \
    net/net-android.c \
    qemu-char.c \
    qemu-log.c \
    qobject/json-lexer.c \
    qobject/json-parser.c \
    qobject/json-streamer.c \
    qobject/qjson.c \
    qobject/qbool.c \
    qobject/qdict.c \
    qobject/qerror.c \
    qobject/qfloat.c \
    qobject/qint.c \
    qobject/qlist.c \
    qobject/qstring.c \
    savevm.c \
    slirp-android/bootp.c \
    slirp-android/cksum.c \
    slirp-android/debug.c \
    slirp-android/if.c \
    slirp-android/ip_icmp.c \
    slirp-android/ip_input.c \
    slirp-android/ip_output.c \
    slirp-android/mbuf.c \
    slirp-android/misc.c \
    slirp-android/sbuf.c \
    slirp-android/slirp.c \
    slirp-android/socket.c \
    slirp-android/tcp_input.c \
    slirp-android/tcp_output.c \
    slirp-android/tcp_subr.c \
    slirp-android/tcp_timer.c \
    slirp-android/tftp.c \
    slirp-android/udp.c \
    ui/console.c \
    ui/d3des.c \
    ui/input.c \
    ui/vnc-android.c \
    util/aes.c \
    util/cutils.c \
    util/error.c \
    util/hexdump.c \
    util/iov.c \
    util/module.c \
    util/notify.c \
    util/osdep.c \
    util/path.c \
    util/qemu-config.c \
    util/qemu-error.c \
    util/qemu-option.c \
    util/qemu-sockets-android.c \
    util/unicode.c \
    util/yield-android.c \

ifeq ($(HOST_ARCH),x86)
    LOCAL_SRC_FILES += disas/i386.c
endif
ifeq ($(HOST_ARCH),x86_64)
    LOCAL_SRC_FILES += disas/i386.c
endif

ifeq ($(HOST_OS),linux)
    LOCAL_SRC_FILES += android/camera/camera-capture-linux.c
endif

ifeq ($(HOST_OS),darwin)
    LOCAL_SRC_FILES += android/camera/camera-capture-mac.m
endif

ifeq ($(HOST_OS),windows)
    LOCAL_SRC_FILES += \
        block/raw-win32.c \
        android/camera/camera-capture-windows.c \
        util/qemu-thread-win32.c \

else
    LOCAL_SRC_FILES += \
        block/raw-posix.c \
        posix-aio-compat.c \
        util/compatfd.c \
        util/qemu-thread-posix.c \

endif

$(call gen-hw-config-defs)
$(call end-emulator-library)


##############################################################################
##############################################################################
###
###  gen-hx-header: Generate headers from .hx file with "hxtool" script.
###
###  The 'hxtool' script is used to generate header files from an input
###  file with the .hx suffix. I.e. foo.hx --> foo.h
###
###  Due to the way the Android build system works, we need to regenerate
###  it for each module (the output will go into a module-specific directory).
###
###  This defines a function that can be used inside a module definition
###
###  $(call gen-hx-header,<input>,<output>,<source-files>)
###
###  Where: <input> is the input file, with a .hx suffix (e.g. foo.hx)
###         <output> is the output file, with a .h or .def suffix
###         <source-files> is a list of source files that include the header
###


gen-hx-header = $(eval $(call gen-hx-header-ev,$1,$2,$3))

define gen-hx-header-ev
intermediates := $$(call local-intermediates-dir)

QEMU_HEADER_H := $$(intermediates)/$$2
$$(QEMU_HEADER_H): PRIVATE_PATH := $$(LOCAL_PATH)
$$(QEMU_HEADER_H): PRIVATE_CUSTOM_TOOL = $$(PRIVATE_PATH)/hxtool -h < $$< > $$@
$$(QEMU_HEADER_H): $$(LOCAL_PATH)/$$1 $$(LOCAL_PATH)/hxtool
	$$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $$(QEMU_HEADER_H)
LOCAL_C_INCLUDES += $$(intermediates)
endef
