# This header contains declarations related to AndroidEmu, the library
# that contains generic code to support Android emulation, independent
# from the rest of QEMU1 or QEMU2. For more details, see:
#
#    external/qemu/docs/ANDROID-EMULATION-LIBRARY.TXT
#
# There are actually several libraries declared here:
#
#    - android-emu-base:
#      Corresponds to the android/base/ and android/utils/ directories
#      which contain a low-level interface over the host platform.
#      None of this code should know or depend on emulation specifics.
#
#    - android-emu:
#      Corresponds to the rest of the AndroidEmu library. It depends on
#      android-emu-base, of course, and any code that should be used by
#      both the QEMU1 and QEMU2 emulator binaries should ideally placed
#      here. Another benefit is that it comes with unit-tests, unlike the
#      rest of the QEMU code bases.
#
#      NOTE: At the moment, this does not include the UI code implemented
#            from android/skin/ sources.
#
#   - android-emu-qemu1 and android-emu-qemu2:
#
#     At the moment, a few AndroidEmu sources still depend on either the UI
#     code, or some QEMU-specific configuration headers, and cannot be moved
#     into android-emu yet. These sources are listed in
#     ANDROID_EMU_DEPENDENT_SOURCES below.
#
#     This code is collected into android-emu-qemu1 and android-emu-qemu2
#     which will be linked against the QEMU1 and QEMU2 binaries in the end.
#
###############################################################################
# public variables

ANDROID_EMU_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

ANDROID_EMU_ROOT := $(LOCAL_PATH)/android

# all includes are like 'android/...', so we need to count on that
ANDROID_EMU_INCLUDES := $(LOCAL_PATH)

# The list of static libraries that anything that depends on android-emu
# should depend on too.
ANDROID_EMU_STATIC_LIBRARIES := \
    android-emu \
    android-emu-base \
    $(LIBCURL_STATIC_LIBRARIES) \
    $(LIBXML2_STATIC_LIBRARIES) \
    $(BREAKPAD_STATIC_LIBRARIES) \
    emulator-libext4_utils \
    emulator-libsparse \
    emulator-libselinux \

ANDROID_EMU_LDLIBS :=

ifneq (windows,$(HOST_OS))
ANDROID_EMU_LDLIBS += -ldl
endif

# Static libraries related to AndroidEmu that need to be linked into QEMU1
ANDROID_EMU_STATIC_LIBRARIES_QEMU1 := \
    android-emu-qemu1 \
    $(ANDROID_EMU_STATIC_LIBRARIES) \

# Static libraries related to AndroidEmu that need to be linked into QEMU1
ANDROID_EMU_STATIC_LIBRARIES_QEMU2 := \
    android-emu-qemu2 \
    $(ANDROID_EMU_STATIC_LIBRARIES) \

###############################################################################
# internal variables to build the libraries

ANDROID_EMU_INTERNAL_CFLAGS := \
    $(EMULATOR_COMMON_CFLAGS) \
    $(LIBXML2_CFLAGS) \
    $(LIBCURL_CFLAGS) \
    $(EMULATOR_VERSION_CFLAGS)

ANDROID_EMU_INTERNAL_QEMU2_CFLAGS := \
    -DANDROID_QEMU2_SPECIFIC \

ANDROID_EMU_INTERNAL_INCLUDES := \
    $(OBJS_DIR)/build \
    $(ANDROID_EMU_INCLUDES) \
    $(LIBPNG_INCLUDES) \
    $(LIBJPEG_INCLUDES) \
    $(LIBCURL_INCLUDES) \
    $(LIBXML2_INCLUDES) \
    $(LIBEXT4_UTILS_INCLUDES) \

ANDROID_EMU_BASE_SOURCES := \
    android/base/async/AsyncReader.cpp \
    android/base/async/AsyncWriter.cpp \
    android/base/async/Looper.cpp \
    android/base/async/ThreadLooper.cpp \
    android/base/containers/PodVector.cpp \
    android/base/containers/PointerSet.cpp \
    android/base/containers/HashUtils.cpp \
    android/base/containers/StringVector.cpp \
    android/base/files/PathUtils.cpp \
    android/base/files/StdioStream.cpp \
    android/base/files/Stream.cpp \
    android/base/files/IniFile.cpp \
    android/base/misc/HttpUtils.cpp \
    android/base/misc/StringUtils.cpp \
    android/base/misc/Utf8Utils.cpp \
    android/base/String.cpp \
    android/base/StringFormat.cpp \
    android/base/StringView.cpp \
    android/base/sockets/SocketDrainer.cpp \
    android/base/sockets/SocketUtils.cpp \
    android/base/sockets/SocketWaiter.cpp \
    android/base/synchronization/MessageChannel.cpp \
    android/base/Log.cpp \
    android/base/memory/LazyInstance.cpp \
    android/base/system/System.cpp \
    android/base/threads/ThreadStore.cpp \
    android/base/Uri.cpp \
    android/base/Version.cpp \
    android/utils/aconfig-file.c \
    android/utils/assert.c \
    android/utils/bufprint.c \
    android/utils/bufprint_system.cpp \
    android/utils/cbuffer.c \
    android/utils/debug.c \
    android/utils/dll.c \
    android/utils/dirscanner.cpp \
    android/utils/eintr_wrapper.c \
    android/utils/filelock.c \
    android/utils/file_data.c \
    android/utils/format.cpp \
    android/utils/host_bitness.cpp \
    android/utils/http_utils.cpp \
    android/utils/iolooper.cpp \
    android/utils/ini.cpp \
    android/utils/intmap.c \
    android/utils/ipaddr.cpp \
    android/utils/lineinput.c \
    android/utils/looper.cpp \
    android/utils/mapfile.c \
    android/utils/misc.c \
    android/utils/panic.c \
    android/utils/path.c \
    android/utils/path_system.cpp \
    android/utils/property_file.c \
    android/utils/reflist.c \
    android/utils/refset.c \
    android/utils/socket_drainer.cpp \
    android/utils/sockets.c \
    android/utils/stralloc.c \
    android/utils/stream.cpp \
    android/utils/string.cpp \
    android/utils/system.c \
    android/utils/system_wrapper.cpp \
    android/utils/tempfile.c \
    android/utils/timezone.c \
    android/utils/uncompress.cpp \
    android/utils/uri.cpp \
    android/utils/utf8_utils.cpp \
    android/utils/vector.c \
    android/utils/x86_cpuid.cpp \

ifeq ($(HOST_OS),windows)
    ANDROID_EMU_BASE_SOURCES += \
        android/base/synchronization/ConditionVariable_win32.cpp \
        android/base/threads/Thread_win32.cpp \
        android/base/system/Win32Utils.cpp \
        android/base/system/Win32UnicodeString.cpp \
        android/utils/win32_cmdline_quote.cpp
else
    ANDROID_EMU_BASE_SOURCES += \
        android/base/threads/Thread_pthread.cpp
endif

ANDROID_EMU_SOURCES := \
    android/adb-qemud.c \
    android/adb-server.c \
    android/android-constants.c \
    android/async-console.c \
    android/async-socket.c \
    android/async-socket-connector.c \
    android/async-utils.c \
    android/avd/hw-config.c \
    android/avd/info.c \
    android/avd/scanner.c \
    android/avd/util.c \
    android/boot-properties.c \
    android/kernel/kernel_utils.cpp \
    android/camera/camera-service.c \
    android/camera/camera-format-converters.c \
    android/cmdline-option.c \
    android/cpu_accelerator.cpp \
    android/console.c \
    android/core-init-utils.c \
    android/curl-support.c \
    android/emulation/android_pipe_pingpong.c \
    android/emulation/android_pipe_throttle.c \
    android/emulation/android_pipe_zero.c \
    android/emulation/android_qemud.cpp \
    android/emulation/bufprint_config_dirs.cpp \
    android/emulation/ConfigDirs.cpp \
    android/emulation/control/LineConsumer.cpp \
    android/emulation/CpuAccelerator.cpp \
    android/emulation/nand_limits.c \
    android/emulation/qemud/android_qemud_client.cpp \
    android/emulation/qemud/android_qemud_multiplexer.cpp \
    android/emulation/qemud/android_qemud_serial.cpp \
    android/emulation/qemud/android_qemud_service.cpp \
    android/emulation/qemud/android_qemud_sink.cpp \
    android/emulation/serial_line.cpp \
    android/emulator-window.c \
    android/filesystems/ext4_resize.cpp \
    android/filesystems/ext4_utils.cpp \
    android/filesystems/fstab_parser.cpp \
    android/filesystems/internal/PartitionConfigBackend.cpp \
    android/filesystems/partition_config.cpp \
    android/filesystems/partition_types.cpp \
    android/filesystems/ramdisk_extractor.cpp \
    android/framebuffer.c \
    android/gps/GpxParser.cpp \
    android/gps/KmlParser.cpp \
    android/gps.c \
    android/gpu_frame.cpp \
    android/help.c \
    android/hw-control.c \
    android/hw-events.c \
    android/hw-fingerprint.c \
    android/hw-kmsg.c \
    android/hw-lcd.c \
    android/hw-pipe-net.c \
    android/hw-qemud.cpp \
    android/hw-sensors.c \
    android/loadpng.c \
    android/main-common-ui.c \
    android/metrics/metrics_reporter.c \
    android/metrics/metrics_reporter_ga.c \
    android/metrics/metrics_reporter_toolbar.c \
    android/metrics/StudioHelper.cpp \
    android/multitouch-screen.c \
    android/multitouch-port.c \
    android/opengl/EmuglBackendList.cpp \
    android/opengl/EmuglBackendScanner.cpp \
    android/opengl/emugl_config.cpp \
    android/opengl/GpuFrameBridge.cpp \
    android/opengles.c \
    android/proxy/proxy_common.c \
    android/proxy/proxy_http.c \
    android/proxy/proxy_http_connector.c \
    android/proxy/proxy_http_rewriter.c \
    android/qemu-tcpdump.c \
    android/qt/qt_path.cpp \
    android/resource.c \
    android/sdk-controller-socket.c \
    android/sensors-port.c \
    android/shaper.c \
    android/snaphost-android.c \
    android/snapshot.c \
    android/telephony/debug.c \
    android/telephony/gsm.c \
    android/telephony/modem.c \
    android/telephony/modem_driver.c \
    android/telephony/remote_call.c \
    android/telephony/sim_card.c \
    android/telephony/sms.c \
    android/telephony/sysdeps.c \
    android/update-check/UpdateChecker.cpp \
    android/update-check/VersionExtractor.cpp \
    android/user-config.c \
    android/utils/jpeg-compress.c \
    android/wear-agent/android_wear_agent.cpp \
    android/wear-agent/WearAgent.cpp \
    android/wear-agent/PairUpWearPhone.cpp \

# Platform-specific camera capture
ifeq ($(HOST_OS),linux)
    ANDROID_EMU_SOURCES += \
        android/camera/camera-capture-linux.c
endif

ifeq ($(HOST_OS),darwin)
    ANDROID_EMU_SOURCES += \
        android/camera/camera-capture-mac.m
endif

ifeq ($(HOST_OS),windows)
    ANDROID_EMU_SOURCES += \
        android/camera/camera-capture-windows.c \
        android/windows_installer.cpp \

endif

# The following source files cannot be moved to android-emu yet for the
# following reasons:
#
#  main-common.c: Depends on UI layer code.
#  qemu-setup.c: Depends on ANDROID_QEMU2_SPECIFIC
#  android_pipe.c: Depends on ANDROID_QEMU2_SPECIFIC
#
# TODO: Move ui-dependent code to android/main-common-ui.c
# TODO: Remove ANDROID_QEMU2_SPECIFIC code path differences.
ANDROID_EMU_DEPENDENT_SOURCES := \
    android/main-common.c \
    android/qemu-setup.c \
    android/emulation/android_pipe.c \

###############################################################################
# now build it

$(call start-emulator-library,android-emu-base)
    LOCAL_SRC_FILES := $(ANDROID_EMU_BASE_SOURCES)
    LOCAL_CFLAGS := $(ANDROID_EMU_INTERNAL_CFLAGS)
    LOCAL_C_INCLUDES := $(ANDROID_EMU_INTERNAL_INCLUDES)
    $(call gen-hw-config-defs)
$(call end-emulator-library)

$(call start-emulator-library,android-emu)
    LOCAL_SRC_FILES := $(ANDROID_EMU_SOURCES)
    LOCAL_CFLAGS := $(ANDROID_EMU_INTERNAL_CFLAGS)
    LOCAL_C_INCLUDES := $(ANDROID_EMU_INTERNAL_INCLUDES)
    $(call gen-hw-config-defs)
$(call end-emulator-library)

$(call start-emulator-library,android-emu-qemu1)
    LOCAL_SRC_FILES := $(ANDROID_EMU_DEPENDENT_SOURCES)
    LOCAL_CFLAGS := $(ANDROID_EMU_INTERNAL_CFLAGS)
    LOCAL_C_INCLUDES := $(ANDROID_EMU_INTERNAL_INCLUDES)
    $(call gen-hw-config-defs)
$(call end-emulator-library)

$(call start-emulator-library,android-emu-qemu2)
    LOCAL_SRC_FILES := $(ANDROID_EMU_DEPENDENT_SOURCES)
    LOCAL_CFLAGS := \
        $(ANDROID_EMU_INTERNAL_CFLAGS) \
        $(ANDROID_EMU_INTERNAL_QEMU2_CFLAGS)

    LOCAL_C_INCLUDES := $(ANDROID_EMU_INTERNAL_INCLUDES)
    $(call gen-hw-config-defs)
$(call end-emulator-library)

LOCAL_PATH := $(ANDROID_EMU_OLD_LOCAL_PATH)

# all done
