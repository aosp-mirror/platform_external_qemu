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
#   - android-emu-ui:
#     Corresponds to the generic UI code, based on the Qt cross-platform
#     library. At the moment, this depends on code that is only provided
#     by the QEMU glue implementations, and isn't unit-testable on its
#     own, which is why it's separated from android-emu.
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
    emulator-libpng \
    emulator-libjpeg \
    $(LIBCURL_STATIC_LIBRARIES) \
    $(LIBXML2_STATIC_LIBRARIES) \
    $(BREAKPAD_STATIC_LIBRARIES) \
    emulator-libext4_utils \
    emulator-libsparse \
    emulator-libselinux \
    emulator-zlib \

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

##############################################################################
##############################################################################
###
###  android-emu-ui: UI specific part of AndroidEmu
###
###

ANDROID_EMU_UI_INCLUDES :=
ANDROID_EMU_UI_LDLIBS :=
ANDROID_EMU_UI_LDFLAGS :=

ANDROID_EMU_UI_STATIC_LIBRARIES :=

ANDROID_EMU_UI_INCLUDES += $(QT_INCLUDES)
ANDROID_EMU_UI_LDFLAGS += $(QT_LDFLAGS)
ANDROID_EMU_UI_LDLIBS += $(QT_LDLIBS)

# the skin support sources
include $(LOCAL_PATH)/android/skin/sources.mk

ifeq ($(HOST_OS),windows)
# For capCreateCaptureWindow used in camera-capture-windows.c
ANDROID_EMU_UI_LDLIBS += -lvfw32
endif

## one for 32-bit
$(call start-emulator-library, android-emu-ui)

LOCAL_CFLAGS := \
    $(EMULATOR_COMMON_CFLAGS) \
    $(LIBXML2_CFLAGS) \
    $(ANDROID_SKIN_CFLAGS) \

# enable MMX code for our skin scaler
ifeq ($(HOST_ARCH),x86)
LOCAL_CFLAGS += -DUSE_MMX=1 -mmmx
endif

LOCAL_C_INCLUDES := \
    $(LIBPNG_INCLUDES) \
    $(LIBJPEG_INCLUDES) \
    $(QT_INCLUDES) \

LOCAL_SRC_FILES += \
    android/gpu_frame.cpp \
    android/emulator-window.c \
    android/resource.c \
    android/user-config.c \
    $(ANDROID_SKIN_SOURCES) \

LOCAL_QT_MOC_SRC_FILES := $(ANDROID_SKIN_QT_MOC_SRC_FILES)
LOCAL_QT_RESOURCES := $(ANDROID_SKIN_QT_RESOURCES)
LOCAL_QT_UI_SRC_FILES := $(ANDROID_SKIN_QT_UI_SRC_FILES)

$(call gen-hw-config-defs)
$(call end-emulator-library)

##############################################################################
##############################################################################
###
###  android-emu unit tests
###
###

$(call start-emulator-program, android_emu$(HOST_SUFFIX)_unittests)

LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(LIBXML2_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \

LOCAL_LDLIBS += $(ANDROID_EMU_LDLIBS)

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
  android/qt/qt_path.cpp \
  android/qt/qt_path_unittest.cpp \
  android/qt/qt_setup.cpp \
  android/qt/qt_setup_unittest.cpp \
  android/telephony/debug.c \
  android/telephony/gsm_unittest.cpp \
  android/telephony/gsm.c \
  android/telephony/sms.c \
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

ifeq (windows,$(HOST_OS))
LOCAL_SRC_FILES += \
  android/base/files/ScopedHandle_unittest.cpp \
  android/base/files/ScopedRegKey_unittest.cpp \
  android/base/system/Win32UnicodeString_unittest.cpp \
  android/base/system/Win32Utils_unittest.cpp \
  android/utils/win32_cmdline_quote_unittest.cpp \
  android/windows_installer_unittest.cpp \

else  # HOST_OS != windows
LOCAL_SRC_FILES += \
  android/emulation/nand_limits_unittest.cpp \

endif  # HOST_OS != windows

LOCAL_CFLAGS += -O0

LOCAL_STATIC_LIBRARIES += \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    emulator-libgtest \
    $(ANDROID_EMU_STATIC_LIBRARIES_QEMU1) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from $(OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)

LOCAL_PATH := $(ANDROID_EMU_OLD_LOCAL_PATH)

$(call start-emulator-program, android_emu$(HOST_SUFFIX)_ui_unittests)

LOCAL_C_INCLUDES += $(EMULATOR_GTEST_INCLUDES)

LOCAL_SRC_FILES := \
    android/skin/keycode_unittest.cpp \
    android/skin/keycode-buffer_unittest.cpp \
    android/skin/rect_unittest.cpp \
    android/skin/region_unittest.cpp \

LOCAL_CFLAGS += -O0
LOCAL_STATIC_LIBRARIES += \
    android-emu-ui \
    $(ANDROID_EMU_STATIC_LIBRARIES_QEMU1) \
    emulator-libgtest \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from $(OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)


# all done
