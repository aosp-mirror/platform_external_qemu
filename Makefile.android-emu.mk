# Declarations related to the AndroidEmu library, which group Android-specific
# emulation features that are used by both QEMU1 and QEMU2 engines.
#
# This file defines the following important modules:
#
#  - android-emu-base
#  - android-emu
#  - android-emu-qemu1
#  - android-emu-qemu2
#
# See below for their documentation. Moreoever, it defines the following
# variables that can be used outside of this script:
#
#   ANDROID_EMU_INCLUDES
#       List of include paths to be used by any module that depends on
#       AndroidEmu
#
#   ANDROID_EMU_STATIC_LIBRARIES
#       List of static libraries to be used by any executable that depends on
#       AndroidEmu.
#
#   ANDROID_EMU_STATIC_LIBRARIES_QEMU1
#       List of static libraries to be used by any QEMU1 executable. Includes
#       the items of ANDROID_EMU_STATIC_LIBRARIES.
#
#   ANDROID_EMU_STATIC_LIBRARIES_QEMU2
#       Same, but for QEMU2 executables.
#
#
#  IMPORTANT: For now, this does not include any declaration related to the UI!

###############################################################################
# public variables

_ANDROID_EMU_OLD_LOCAL_PATH := $(LOCAL_PATH)

_ANDROID_EMU_ROOT := $(call my-dir)

# all includes are like 'android/...', so we need to count on that
ANDROID_EMU_INCLUDES := $(_ANDROID_EMU_ROOT)

###############################################################################
#
#  android-emu-base
#
#  This is a static library containing all the low-level system interfaces
#  provided by android/base/ and android/utils/. It should only depend on
#  system headers and libraries, and nothing else (including the C++ STL).
#

$(call start-emulator-library,android-emu-base)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(EMULATOR_COMMON_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \

LOCAL_SRC_FILES := \
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
    android/base/threads/Async.cpp \
    android/base/threads/FunctorThread.cpp \
    android/base/threads/ThreadStore.cpp \
    android/base/Uri.cpp \
    android/base/Version.cpp \
    android/utils/aconfig-file.c \
    android/utils/assert.c \
    android/utils/async.cpp \
    android/utils/bufprint.c \
    android/utils/bufprint_system.cpp \
    android/utils/cbuffer.c \
    android/utils/debug.c \
    android/utils/dll.c \
    android/utils/dirscanner.cpp \
    android/utils/eintr_wrapper.c \
    android/utils/exec.cpp \
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
    android/utils/uri.cpp \
    android/utils/utf8_utils.cpp \
    android/utils/vector.c \
    android/utils/x86_cpuid.cpp \

ifeq ($(HOST_OS),windows)
LOCAL_SRC_FILES += \
    android/base/synchronization/ConditionVariable_win32.cpp \
    android/base/threads/Thread_win32.cpp \
    android/base/system/Win32Utils.cpp \
    android/base/system/Win32UnicodeString.cpp \
    android/utils/win32_cmdline_quote.cpp \

else
LOCAL_SRC_FILES += \
    android/base/threads/Thread_pthread.cpp \

endif

$(call end-emulator-library)

###############################################################################
#
#  android-emu
#
#  This is a static library containing all the Android emulation features
#  shared with both the QEMU1 and QEMU2 engines. It should only depend on
#  android-emu-base, a few third-party prebuilt libraries (e.g. libxml2) and
#  system headers and libraries.
#
#  NOTE: This does not include UI code, and comes with its own set of unit
#        tests.
#

# Common CFLAGS for android-emu related sources.
_ANDROID_EMU_INTERNAL_CFLAGS := \
    $(EMULATOR_COMMON_CFLAGS) \
    $(LIBCURL_CFLAGS) \
    $(LIBXML2_CFLAGS) \

# Common INCLUDES for android-emu related sources.
_ANDROID_EMU_INTERNAL_INCLUDES := \
    $(EMULATOR_COMMON_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \
    $(BREAKPAD_CLIENT_INCLUDES) \
    $(LIBCURL_INCLUDES) \
    $(LIBJPEG_INCLUDES) \
    $(LIBXML2_INCLUDES) \
    $(LIBEXT4_UTILS_INCLUDES) \
    $(LIBPNG_INCLUDES) \
    $(ZLIB_INCLUDES) \


$(call start-emulator-library,android-emu)

LOCAL_CFLAGS := $(_ANDROID_EMU_INTERNAL_CFLAGS)

LOCAL_C_INCLUDES := $(_ANDROID_EMU_INTERNAL_INCLUDES)

LOCAL_SRC_FILES := \
    android/adb-qemud.c \
    android/adb-server.c \
    android/android-constants.c \
    android/avd/hw-config.c \
    android/avd/info.c \
    android/avd/scanner.c \
    android/avd/util.c \
    android/async-console.c \
    android/async-socket.c \
    android/async-socket-connector.c \
    android/async-utils.c \
    android/boot-properties.c \
    android/camera/camera-service.c \
    android/camera/camera-format-converters.c \
    android/cmdline-option.c \
    android/console.c \
    android/cpu_accelerator.cpp \
    android/crashreport/CrashSystem.cpp \
    android/crashreport/CrashReporter_common.cpp \
    android/crashreport/CrashReporter_$(HOST_OS).cpp \
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
    android/gps.c \
    android/gpu_frame.cpp \
    android/help.c \
    android/hw-control.c \
    android/hw-events.c \
    android/hw-fingerprint.c \
    android/hw-pipe-net.c \
    android/hw-qemud.cpp \
    android/hw-sensors.c \
    android/jpeg-compress.c \
    android/kernel/kernel_utils.cpp \
    android/loadpng.c \
    android/main-common-ui.c \
    android/metrics/metrics_reporter.cpp \
    android/metrics/metrics_reporter_ga.cpp \
    android/metrics/metrics_reporter_toolbar.cpp \
    android/metrics/StudioHelper.cpp \
    android/multitouch-port.c \
    android/multitouch-screen.c \
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
    android/uncompress.cpp \
    android/update-check/UpdateChecker.cpp \
    android/update-check/VersionExtractor.cpp \
    android/user-config.c \

# Platform-specific camera capture
ifeq ($(HOST_OS),linux)
    LOCAL_SRC_FILES += \
        android/camera/camera-capture-linux.c
endif

ifeq ($(HOST_OS),darwin)
    LOCAL_SRC_FILES += \
        android/camera/camera-capture-mac.m
endif

ifeq ($(HOST_OS),windows)
    LOCAL_SRC_FILES += \
        android/camera/camera-capture-windows.cpp \
        android/windows_installer.cpp \

endif

$(call gen-hw-config-defs)
$(call end-emulator-library)

# List of static libraries that anything that depends on android-emu
# should use.
ANDROID_EMU_STATIC_LIBRARIES := \
    android-emu \
    android-emu-base \

# TODO(digit): Move AndroidEmu unit tests declarations here.

###############################################################################
#
#  android-emu-qemu1 and android-emu-qemu2
#
#  Some of the sources under android/ are logically part of AndroidEmu
#  but cannot be part of android-emu yet due to the following reasons:
#
#  - They compile differently based on the ANDROID_QEMU2_SPECIFIC macro.
#  - They depend on Android UI code that is not part of android-emu.
#
#  These sources are grouped into the android-emu-qemu1 and android-emu-qemu2
#  static libraries. The plan is to remove these completely in the future.
#

# TODO(digit): main-common.c: Move non-UI dependent code to main-common-ui.c
# TODO(digit): Remove ANDROID_QEMU2_SPECIFIC from qemu-setup.c and
#              android_pipe.c
_ANDROID_EMU_DEPENDENT_SOURCES := \
    android/main-common.c \
    android/qemu-setup.c \
    android/emulation/android_pipe.c \

$(call start-emulator-library,android-emu-qemu1)
LOCAL_CFLAGS := $(_ANDROID_EMU_INTERNAL_CFLAGS)
LOCAL_C_INCLUDES := $(_ANDROID_EMU_INTERNAL_INCLUDES)
LOCAL_SRC_FILES := $(_ANDROID_EMU_DEPENDENT_SOURCES)
$(call gen-hw-config-defs)
$(call end-emulator-library)

$(call start-emulator-library,android-emu-qemu2)
LOCAL_CFLAGS := $(_ANDROID_EMU_INTERNAL_CFLAGS)
LOCAL_CFLAGS += -DANDROID_QEMU2_SPECIFIC
LOCAL_C_INCLUDES := $(_ANDROID_EMU_INTERNAL_INCLUDES)
LOCAL_SRC_FILES := $(_ANDROID_EMU_DEPENDENT_SOURCES)
$(call gen-hw-config-defs)
$(call end-emulator-library)

# List of static libraries that anything that depends on android-emu-qemu1
# should use.
ANDROID_EMU_STATIC_LIBRARIES_QEMU1 := \
    android-emu-qemu1 \
    $(ANDROID_EMU_STATIC_LIBRARIES) \

# List of static libraries that anything that depends on android-emu-qemu2
# should use.
ANDROID_EMU_STATIC_LIBRARIES_QEMU2 := \
    android-emu-qemu2 \
    $(ANDROID_EMU_STATIC_LIBRARIES) \


# Done

LOCAL_PATH := $(_ANDROID_EMU_OLD_LOCAL_PATH)
