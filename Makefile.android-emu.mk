# Declarations related to the AndroidEmu library, which group Android-specific
# emulation features that are used by both QEMU1 and QEMU2 engines.
#
# This file defines the following important modules:
#
#  - android-emu-base
#  - android-emu
#  - emulator-libui
#
# And corresponding unit-tests.
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
#   ANDROID_EMU_LDLIBS
#       List of system libraries to be used by any executable or shared library
#       that depends on AndroidEmu.
#

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
    android/utils/dns.cpp \
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

ifeq ($(BUILD_TARGET_OS),windows)
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
    android/core-init-utils.c \
    android/cpu_accelerator.cpp \
    android/crashreport/CrashSystem.cpp \
    android/crashreport/CrashReporter_common.cpp \
    android/crashreport/CrashReporter_$(BUILD_TARGET_OS).cpp \
    android/curl-support.c \
    android/emulation/android_pipe.c \
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
    android/jpeg-compress.c \
    android/kernel/kernel_utils.cpp \
    android/loadpng.c \
    android/main-common.c \
    android/metrics/IniFileAutoFlusher.cpp \
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
    android/qemu-setup.c \
    android/qemu-tcpdump.c \
    android/qt/qt_path.cpp \
    android/qt/qt_setup.cpp \
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
    android/wear-agent/android_wear_agent.cpp \
    android/wear-agent/WearAgent.cpp \
    android/wear-agent/PairUpWearPhone.cpp \

# Platform-specific camera capture
ifeq ($(BUILD_TARGET_OS),linux)
    LOCAL_SRC_FILES += \
        android/camera/camera-capture-linux.c
endif

ifeq ($(BUILD_TARGET_OS),darwin)
    LOCAL_SRC_FILES += \
        android/camera/camera-capture-mac.m
endif

ifeq ($(BUILD_TARGET_OS),windows)
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
    $(LIBCURL_STATIC_LIBRARIES) \
    $(LIBXML2_STATIC_LIBRARIES) \
    $(BREAKPAD_CLIENT_STATIC_LIBRARIES) \
    emulator-libext4_utils \
    emulator-libsparse \
    emulator-libselinux \
    emulator-libjpeg \
    emulator-libpng \
    emulator-zlib \

ANDROID_EMU_LDLIBS := \
    $(LIBCURL_LDLIBS) \
    $(BREAKPAD_CLIENT_LDLIBS) \

ifeq ($(BUILD_TARGET_OS),windows)
# For capCreateCaptureWindow used in camera-capture-windows.cpp
ANDROID_EMU_LDLIBS += -lvfw32
endif

###############################################################################
#
#  android-emu unit tests
#
#

$(call start-emulator-program, android_emu$(BUILD_TARGET_SUFFIX)_unittests)

LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \
    $(LIBXML2_INCLUDES) \

LOCAL_LDLIBS += \
    $(ANDROID_EMU_LDLIBS) \

LOCAL_SRC_FILES := \
  android/avd/util_unittest.cpp \
  android/base/containers/HashUtils_unittest.cpp \
  android/base/containers/PodVector_unittest.cpp \
  android/base/containers/PointerSet_unittest.cpp \
  android/base/containers/ScopedPointerSet_unittest.cpp \
  android/base/containers/StringVector_unittest.cpp \
  android/base/containers/TailQueueList_unittest.cpp \
  android/base/EintrWrapper_unittest.cpp \
  android/base/files/IniFile_unittest.cpp \
  android/base/files/PathUtils_unittest.cpp \
  android/base/files/ScopedFd_unittest.cpp \
  android/base/files/ScopedStdioFile_unittest.cpp \
  android/base/files/Stream_unittest.cpp \
  android/base/Log_unittest.cpp \
  android/base/memory/LazyInstance_unittest.cpp \
  android/base/memory/MallocUsableSize_unittest.cpp \
  android/base/memory/ScopedPtr_unittest.cpp \
  android/base/memory/QSort_unittest.cpp \
  android/base/misc/HttpUtils_unittest.cpp \
  android/base/misc/StringUtils_unittest.cpp \
  android/base/misc/Utf8Utils_unittest.cpp \
  android/base/sockets/ScopedSocket_unittest.cpp \
  android/base/sockets/SocketDrainer_unittest.cpp \
  android/base/sockets/SocketWaiter_unittest.cpp \
  android/base/String_unittest.cpp \
  android/base/StringFormat_unittest.cpp \
  android/base/StringView_unittest.cpp \
  android/base/synchronization/ConditionVariable_unittest.cpp \
  android/base/synchronization/Lock_unittest.cpp \
  android/base/synchronization/MessageChannel_unittest.cpp \
  android/base/system/System_unittest.cpp \
  android/base/threads/FunctorThread_unittest.cpp \
  android/base/threads/Thread_unittest.cpp \
  android/base/threads/ThreadStore_unittest.cpp \
  android/base/Uri_unittest.cpp \
  android/base/Version_unittest.cpp \
  android/emulation/bufprint_config_dirs_unittest.cpp \
  android/emulation/ConfigDirs_unittest.cpp \
  android/emulation/control/LineConsumer_unittest.cpp \
  android/emulation/CpuAccelerator_unittest.cpp \
  android/emulation/serial_line_unittest.cpp \
  android/filesystems/ext4_utils_unittest.cpp \
  android/filesystems/fstab_parser_unittest.cpp \
  android/filesystems/partition_config_unittest.cpp \
  android/filesystems/partition_types_unittest.cpp \
  android/filesystems/ramdisk_extractor_unittest.cpp \
  android/filesystems/testing/TestSupport.cpp \
  android/gps/GpxParser_unittest.cpp \
  android/gps/internal/GpxParserInternal_unittest.cpp \
  android/gps/internal/KmlParserInternal_unittest.cpp \
  android/gps/KmlParser_unittest.cpp \
  android/kernel/kernel_utils_unittest.cpp \
  android/metrics/metrics_reporter_unittest.cpp \
  android/metrics/metrics_reporter_ga_unittest.cpp \
  android/metrics/metrics_reporter_toolbar_unittest.cpp \
  android/metrics/StudioHelper_unittest.cpp \
  android/opengl/EmuglBackendList_unittest.cpp \
  android/opengl/EmuglBackendScanner_unittest.cpp \
  android/opengl/emugl_config_unittest.cpp \
  android/opengl/GpuFrameBridge_unittest.cpp \
  android/qt/qt_path_unittest.cpp \
  android/qt/qt_setup_unittest.cpp \
  android/telephony/gsm_unittest.cpp \
  android/telephony/sms_unittest.cpp \
  android/update-check/UpdateChecker_unittest.cpp \
  android/update-check/VersionExtractor_unittest.cpp \
  android/utils/aconfig-file_unittest.cpp \
  android/utils/bufprint_unittest.cpp \
  android/utils/dirscanner_unittest.cpp \
  android/utils/eintr_wrapper_unittest.cpp \
  android/utils/file_data_unittest.cpp \
  android/utils/format_unittest.cpp \
  android/utils/host_bitness_unittest.cpp \
  android/utils/path_unittest.cpp \
  android/utils/property_file_unittest.cpp \
  android/utils/string_unittest.cpp \
  android/utils/x86_cpuid_unittest.cpp \
  android/wear-agent/PairUpWearPhone_unittest.cpp \
  android/wear-agent/testing/WearAgentTestUtils.cpp \
  android/wear-agent/WearAgent_unittest.cpp \

ifeq (windows,$(BUILD_TARGET_OS))
LOCAL_SRC_FILES += \
  android/base/files/ScopedHandle_unittest.cpp \
  android/base/files/ScopedRegKey_unittest.cpp \
  android/base/system/Win32UnicodeString_unittest.cpp \
  android/base/system/Win32Utils_unittest.cpp \
  android/utils/win32_cmdline_quote_unittest.cpp \
  android/windows_installer_unittest.cpp \

else
LOCAL_SRC_FILES += \
  android/emulation/nand_limits_unittest.cpp \

endif

LOCAL_CFLAGS += -O0

LOCAL_STATIC_LIBRARIES += \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    emulator-libgtest \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from
# $(BUILD_OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)

##############################################################################
#
#  emulator-libui
#
#  This is the library that implements the emulator's UI on top of
#  android-emu. Note that it depends on interfaces that must be implemented
#  by the engine-specific glue code. As such, the code cannot be unit-tested
#  for now.
#
EMULATOR_LIBUI_INCLUDES :=
EMULATOR_LIBUI_LDLIBS :=
EMULATOR_LIBUI_LDFLAGS :=
EMULATOR_LIBUI_STATIC_LIBRARIES :=

EMULATOR_LIBUI_INCLUDES += $(QT_INCLUDES)
EMULATOR_LIBUI_LDFLAGS += $(QT_LDFLAGS)
EMULATOR_LIBUI_LDLIBS += $(QT_LDLIBS)

# The skin support sources
include $(LOCAL_PATH)/android/skin/sources.mk

$(call start-emulator-library, emulator-libui)

LOCAL_CFLAGS += \
    $(EMULATOR_COMMON_CFLAGS) \
    $(ANDROID_SKIN_CFLAGS) \
    $(LIBXML2_CFLAGS) \

LOCAL_C_INCLUDES := \
    $(EMULATOR_COMMON_INCLUDES) \
    $(EMULATOR_LIBUI_INCLUDES) \

LOCAL_SRC_FILES += \
    $(ANDROID_SKIN_SOURCES) \
    android/gpu_frame.cpp \
    android/emulator-window.c \
    android/main-common-ui.c \
    android/resource.c \
    android/user-config.c \

LOCAL_QT_MOC_SRC_FILES := $(ANDROID_SKIN_QT_MOC_SRC_FILES)
LOCAL_QT_RESOURCES := $(ANDROID_SKIN_QT_RESOURCES)
LOCAL_QT_UI_SRC_FILES := $(ANDROID_SKIN_QT_UI_SRC_FILES)
$(call gen-hw-config-defs)
$(call end-emulator-library)

# emulator-libui unit tests

$(call start-emulator-program, emulator$(BUILD_TARGET_SUFFIX)_libui_unittests)

LOCAL_C_INCLUDES += \
    $(EMULATOR_COMMON_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \

LOCAL_SRC_FILES := \
    android/skin/keycode_unittest.cpp \
    android/skin/keycode-buffer_unittest.cpp \
    android/skin/rect_unittest.cpp \
    android/skin/region_unittest.cpp \

LOCAL_C_INCLUDES += \
    $(LIBXML2_INCLUDES) \

LOCAL_CFLAGS += -O0
LOCAL_STATIC_LIBRARIES += \
    emulator-libui \
    emulator-libgtest \
    $(ANDROID_EMU_STATIC_LIBRARIES) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from
# $(BUILD_OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)
$(call gen-hw-config-defs)

$(call end-emulator-program)

# Done

LOCAL_PATH := $(_ANDROID_EMU_OLD_LOCAL_PATH)
