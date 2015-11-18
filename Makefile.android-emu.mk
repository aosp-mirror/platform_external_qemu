###############################################################################
# public variables

ANDROID_EMU_ROOT := $(SRC_PATH)/android

# all includes are like 'android/...', so we need to count on that
ANDROID_EMU_INCLUDES := \
    $(ANDROID_EMU_ROOT)/.. \

ANDROID_EMU_STATIC_LIBRARIES_QEMU1 := \
    android-emu-qemu1 \
    android-emu \
    android-emu-base \

ANDROID_EMU_STATIC_LIBRARIES_QEMU2 := \
    android-emu-qemu2 \
    android-emu \
    android-emu-base \

# a lightweight version, without the very qemu-specific stuff
ANDROID_EMU_BASE_STATIC_LIBRARIES_QEMU1 := \
    android-emu-qemu1 \
    android-emu-base \

ANDROID_EMU_BASE_STATIC_LIBRARIES_QEMU2 := \
    android-emu-qemu2 \
    android-emu-base \

###############################################################################
# internal variables to build the libraries

ANDROID_EMU_INTERNAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS) $(LIBXML2_CFLAGS)
ANDROID_EMU_INTERNAL_QEMU2_CFLAGS := \
    -DANDROID_QEMU2_SPECIFIC \
    -DANDROID_QEMU2_INTEGRATED_BUILD \

ifndef EMULATOR_USE_QT
    # skin is only used in QT version of QEMU2 for now, so let's make sure
    # it's not compiled into the non-QT one
    ANDROID_EMU_INTERNAL_QEMU2_CFLAGS += -DNO_SKIN
endif

ANDROID_EMU_INTERNAL_INCLUDES := \
    $(OBJS_DIR)/build \
    $(ANDROID_EMU_INCLUDES) \
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
    android/camera/camera-service.c \
    android/camera/camera-format-converters.c \
    android/filesystems/ext4_resize.cpp \
    android/filesystems/ext4_utils.cpp \
    android/filesystems/fstab_parser.cpp \
    android/filesystems/internal/PartitionConfigBackend.cpp \
    android/filesystems/partition_config.cpp \
    android/filesystems/partition_types.cpp \
    android/filesystems/ramdisk_extractor.cpp \
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

# Platform-specific camera capture
ifeq ($(HOST_OS),linux)
    ANDROID_EMU_BASE_SOURCES += \
        android/camera/camera-capture-linux.c
endif

ifeq ($(HOST_OS),darwin)
    ANDROID_EMU_BASE_SOURCES += \
        android/camera/camera-capture-mac.m
endif

ifeq ($(HOST_OS),windows)
    ANDROID_EMU_BASE_SOURCES += \
        android/camera/camera-capture-windows.c
endif

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
    android/async-console.c \
    android/async-socket.c \
    android/async-socket-connector.c \
    android/async-utils.c \
    android/boot-properties.c \
    android/console.c \
    android/curl-support.c \
    android/emulation/android_pipe_pingpong.c \
    android/emulation/android_pipe_throttle.c \
    android/emulation/android_pipe_zero.c \
    android/emulation/android_qemud.cpp \
    android/emulation/control/LineConsumer.cpp \
    android/emulation/nand_limits.c \
    android/emulation/qemud/android_qemud_client.cpp \
    android/emulation/qemud/android_qemud_multiplexer.cpp \
    android/emulation/qemud/android_qemud_serial.cpp \
    android/emulation/qemud/android_qemud_service.cpp \
    android/emulation/qemud/android_qemud_sink.cpp \
    android/emulation/serial_line.cpp \
    android/emulator-window.c \
    android/framebuffer.c \
    android/gps.c \
    android/gpu_frame.cpp \
    android/hw-control.c \
    android/hw-events.c \
    android/hw-fingerprint.c \
    android/hw-pipe-net.c \
    android/hw-qemud.cpp \
    android/hw-sensors.c \
    android/loadpng.c \
    android/main-common-ui.c \
    android/metrics/metrics_reporter.c \
    android/metrics/metrics_reporter_ga.c \
    android/metrics/metrics_reporter_toolbar.c \
    android/metrics/StudioHelper.cpp \
    android/opengles.c \
    android/opengl/GpuFrameBridge.cpp \
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

ANDROID_EMU_DEPENDENT_SOURCES := \
    android/android-constants.c \
    android/cmdline-option.c \
    android/cpu_accelerator.cpp \
    android/help.c \
    android/main-common.c \
    android/qemu-setup.c \
    android/avd/hw-config.c \
    android/avd/info.c \
    android/avd/scanner.c \
    android/avd/util.c \
    android/emulation/android_pipe.c \
    android/emulation/bufprint_config_dirs.cpp \
    android/emulation/ConfigDirs.cpp \
    android/emulation/CpuAccelerator.cpp \
    android/kernel/kernel_utils.cpp \
    android/opengl/EmuglBackendList.cpp \
    android/opengl/EmuglBackendScanner.cpp \
    android/opengl/emugl_config.cpp \

ifeq (windows,$(HOST_OS))
ANDROID_EMU_DEPENDENT_SOURCES += \
    android/windows_installer.cpp \

endif

###############################################################################
# now build it

ANDROID_EMU_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(SRC_PATH)

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
