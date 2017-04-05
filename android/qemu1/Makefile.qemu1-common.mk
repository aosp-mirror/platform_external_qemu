# When building this project, we actually generate several components which
# are the following:
#
#  - the emulator-ui program (which is target-agnostic)
#  - the "standalone" emulator programs (embed both UI and engine in a single
#    binary and process), i.e. "emulator" for ARM and "emulator-x86" for x86.
#
# This file defines static host libraries that will be used by at least two
# of these components.
#

QEMU1_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

QEMU1_COMMON_CFLAGS := \
    $(EMULATOR_COMMON_CFLAGS) \

QEMU1_COMMON_INCLUDES := \
    $(EMULATOR_COMMON_INCLUDES) \
    $(LOCAL_PATH)/include

# Need to include "qapi-types.h" and other auto-generated files from
# android/configure.sh
QEMU1_COMMON_INCLUDES += $(BUILD_OBJS_DIR)/build/qemu1-qapi-auto-generated

# Zlib sources
QEMU1_COMMON_INCLUDES += $(ZLIB_INCLUDES)

# GLib sources
QEMU1_COMMON_INCLUDES += $(MINIGLIB_INCLUDES)

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

ifeq ($(BUILD_TARGET_OS),darwin)
  CONFIG_COREAUDIO ?= yes
  AUDIO_CFLAGS += -DHOST_BSD=1
endif

ifeq ($(BUILD_TARGET_OS),windows)
  CONFIG_WINAUDIO ?= yes
endif

ifeq ($(BUILD_TARGET_OS),linux)
  CONFIG_OSS  ?= yes
  CONFIG_ALSA ?= yes
  CONFIG_PULSEAUDIO ?= yes
  CONFIG_ESD  ?= yes
endif

ifeq ($(BUILD_TARGET_OS),freebsd)
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
ifneq ($(BUILD_TARGET_OS),windows)
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
    $(QEMU1_COMMON_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \
    $(LOCAL_PATH)/slirp-android \
    $(intermediates) \
    $(LIBJPEG_INCLUDES) \

LOCAL_CFLAGS := \
    $(QEMU1_COMMON_CFLAGS) \
    -W \
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
ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_CFLAGS += -fno-stack-protector
endif

LOCAL_SRC_FILES += \
    $(AUDIO_SOURCES) \
    aio-android.c \
    android-qemu1-glue/android_qemud.cpp \
    android-qemu1-glue/audio-test.c \
    android-qemu1-glue/base/async/Looper.cpp \
    android-qemu1-glue/base/files/QemuFileStream.cpp \
    android-qemu1-glue/display.c \
    android-qemu1-glue/emulation/charpipe.c \
    android-qemu1-glue/emulation/CharSerialLine.cpp \
    android-qemu1-glue/emulation/serial_line.cpp \
    android-qemu1-glue/emulation/VmLock.cpp \
    android-qemu1-glue/looper-qemu.cpp \
    android-qemu1-glue/qemu-cellular-agent-impl.c \
    android-qemu1-glue/qemu-display-agent-impl.cpp \
    android-qemu1-glue/qemu-finger-agent-impl.c \
    android-qemu1-glue/qemu-location-agent-impl.c \
    android-qemu1-glue/qemu-net-agent-impl.c \
    android-qemu1-glue/qemu-car-data-agent-impl.cpp \
    android-qemu1-glue/qemu-sensors-agent-impl.c \
    android-qemu1-glue/qemu-setup.cpp \
    android-qemu1-glue/qemu-telephony-agent-impl.c \
    android-qemu1-glue/qemu-window-agent-impl.c \
    android-qemu1-glue/telephony/modem_init.cpp \
    android-qemu1-glue/utils/stream.cpp \
    async.c \
    audio/audio.c \
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
    slirp-android/dhcpv6.c \
    slirp-android/if.c \
    slirp-android/ip_icmp.c \
    slirp-android/ip6_icmp.c \
    slirp-android/ip_input.c \
    slirp-android/ip6_input.c \
    slirp-android/ip_output.c \
    slirp-android/ip6_output.c \
    slirp-android/mbuf.c \
    slirp-android/misc.c \
    slirp-android/ndp_table.c \
    slirp-android/sbuf.c \
    slirp-android/slirp.c \
    slirp-android/socket.c \
    slirp-android/tcp_input.c \
    slirp-android/tcp_output.c \
    slirp-android/tcp_subr.c \
    slirp-android/tcp_timer.c \
    slirp-android/tftp.c \
    slirp-android/udp.c \
    slirp-android/udp6.c \
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

ifeq ($(BUILD_TARGET_ARCH),x86)
    LOCAL_SRC_FILES += disas/i386.c
endif
ifeq ($(BUILD_TARGET_ARCH),x86_64)
    LOCAL_SRC_FILES += disas/i386.c
endif

ifeq ($(BUILD_TARGET_OS),windows)
    LOCAL_SRC_FILES += \
        block/raw-win32.c \
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

LOCAL_PATH := $(QEMU1_OLD_LOCAL_PATH)
