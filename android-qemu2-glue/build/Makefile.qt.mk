##############################################################################
# Libxml2 definitions
#
LIBXML2_TOP_DIR := $(LIBXML2_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)
LIBXML2_INCLUDES := $(LIBXML2_TOP_DIR)/include
LIBXML2_LDLIBS := $(LIBXML2_TOP_DIR)/lib/libxml2.a

# Required on Windows to indicate that the code will link against a static
# version of libxml2. Otherwise, the linker complains about undefined
# references to '_imp__xmlFree'.
LIBXML2_CFLAGS := -DLIBXML_STATIC


# qt definitions
    QT_TOP_DIR := $(QT_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)
    QT_TOP64_DIR := $(QT_PREBUILTS_DIR)/$(HOST_OS)-x86_64
    QT_MOC_TOOL := $(QT_TOP64_DIR)/bin/moc
    QT_RCC_TOOL := $(QT_TOP64_DIR)/bin/rcc
    # Special-case: the 'uic' tool depends on Qt5Core: always ensure that the
    # version that is being used is from the prebuilts directory. Otherwise
    # the executable may fail to start due to dynamic linking problems.
    QT_UIC_TOOL_LDPATH := $(QT_TOP64_DIR)/lib
    QT_UIC_TOOL := $(QT_TOP64_DIR)/bin/uic

    EMULATOR_QT_LIBS := Qt5Widgets Qt5Gui Qt5Core
    EMULATOR_QT_LDLIBS := $(foreach lib,$(EMULATOR_QT_LIBS),-l$(lib))
    ifeq ($(HOST_OS),windows)
        # On Windows, linking to mingw32 is required. The library is provided
        # by the toolchain, and depends on a main() function provided by qtmain
        # which itself depends on qMain(). These must appear in LDFLAGS and
        # not LDLIBS since qMain() is provided by object/libraries that
        # appear after these in the link command-line.
        EMULATOR_QT_LDFLAGS += \
                -L$(QT_TOP_DIR)/bin \
                -lmingw32 \
                $(QT_TOP_DIR)/lib/libqtmain.a
    else
        EMULATOR_QT_LDFLAGS := -L$(QT_TOP_DIR)/lib
    endif
    QT_INCLUDE := $(QT_PREBUILTS_DIR)/common/include
    ANDROID_QT_CFLAGS := \
            -I$(QT_INCLUDE) \
            -I$(QT_INCLUDE)/QtCore \
            -I$(QT_INCLUDE)/QtGui \
            -I$(QT_INCLUDE)/QtWidgets \
            -DQEMU2

    ANDROID_QT_LDFLAGS := $(EMULATOR_QT_LDFLAGS)
    ANDROID_QT_LDLIBS := $(EMULATOR_QT_LDLIBS)

    ANDROID_QT_CFLAGS += $(LIBXML2_CFLAGS)


ANDROID_SKIN_LDLIBS :=
ANDROID_SKIN_LDFLAGS :=
ANDROID_SKIN_CFLAGS :=

ANDROID_SKIN_QT_MOC_SRC_FILES :=
ANDROID_SKIN_QT_RESOURCES :=

ANDROID_SKIN_SOURCES := \
    android/skin/charmap.c \
    android/skin/rect.c \
    android/skin/region.c \
    android/skin/image.c \
    android/skin/trackball.c \
    android/skin/keyboard.c \
    android/skin/keycode.c \
    android/skin/keycode-buffer.c \
    android/skin/keyset.c \
    android/skin/file.c \
    android/skin/window.c \
    android/skin/resource.c \
    android/skin/scaler.c \
    android/skin/ui.c \

# Definitions related to the Qt-based UI backend.

ANDROID_SKIN_SOURCES += \
    android/skin/qt/event-qt.cpp \
    android/skin/qt/surface-qt.cpp \
    android/skin/qt/winsys-qt.cpp \

ifeq (darwin,$(HOST_OS))
ANDROID_SKIN_SOURCES += \
    android/skin/qt/mac-native-window.mm
endif

#include $(LOCAL_PATH)/../qemu/distrib/libpng.mk
#include $(LOCAL_PATH)/../qemu/distrib/jpeg-6b/libjpeg.mk

ANDROID_SKIN_CFLAGS += \
    -I$(LIBXML2_INCLUDES) \
    -I$(LIBCURL_INCLUDES) \
    $(ANDROID_QT_CFLAGS) \
    $(LIBPNG_CFLAGS)

ANDROID_SKIN_STATIC_LIBRARIES += \
    emulator-libpng \
    emulator-libjpeg

ANDROID_SKIN_LDLIBS += \
    $(LIBXML2_LDLIBS) \
    $(ANDROID_QT_LDLIBS) \
    $(LIBCURL_LDLIBS) \

ANDROID_SKIN_LDFLAGS += $(ANDROID_QT_LDFLAGS)

ANDROID_SKIN_SOURCES += \
    android/gps/GpxParser.cpp \
    android/gps/KmlParser.cpp \
    android/skin/qt/editable-slider-widget.cpp \
    android/skin/qt/emulator-qt-window.cpp \
    android/skin/qt/ext-battery.cpp \
    android/skin/qt/ext-cellular.cpp \
    android/skin/qt/ext-dpad.cpp \
    android/skin/qt/ext-finger.cpp \
    android/skin/qt/ext-keyboard-shortcuts.cpp \
    android/skin/qt/ext-location.cpp \
    android/skin/qt/ext-settings.cpp \
    android/skin/qt/ext-sms.cpp \
    android/skin/qt/ext-telephony.cpp \
    android/skin/qt/ext-virtsensors.cpp \
    android/skin/qt/extended-window.cpp \
    android/skin/qt/tool-window.cpp \

ANDROID_SKIN_QT_MOC_SRC_FILES := \
    android/skin/qt/editable-slider-widget.h \
    android/skin/qt/emulator-qt-window.h \
    android/skin/qt/extended-window.h \
    android/skin/qt/tool-window.h \

ANDROID_SKIN_QT_RESOURCES := \
    android/skin/qt/resources.qrc \

ANDROID_SKIN_QT_UI_SRC_FILES := \
    android/skin/qt/extended.ui \
    android/skin/qt/tools.ui \

#android emu
ANDROID_SKIN_SOURCES += \
    android/emulator-window.c \
    android/opengles.c \
    android/qt/qt_path.cpp \
    android/main-common-ui.c \
    android/loadpng.c \
    android/console.c \
    android/resource.c \
    android/user-config.c \
    android/qemu-setup.c \
    android/sensors-port.c \
    android/sdk-controller-socket.c \
    android/async-socket.c \
    android/async-socket-connector.c \
    android/hw-sensors.c \
    android/hw-events.c \
    android/hw-fingerprint.c \
    android/hw-control.c \
    android/hw-pipe-net.c \
    android/gpu_frame.cpp \
    android/qemu-tcpdump.c \
    android/adb-qemud.c \
    android/adb-server.c \
    android/shaper.c \
    android/telephony/sms.c \
    android/telephony/gsm.c \
    android/telephony/debug.c \
    android/telephony/modem.c \
    android/telephony/modem_driver.c \
    android/telephony/sysdeps.c \
    android/telephony/sim_card.c \
    android/telephony/remote_call.c \

ANDROID_SKIN_SOURCES += \
    android/android-constants.c \
    android/async-console.c \
    android/async-utils.c \
    android/curl-support.c \
    android/framebuffer.c \
    android/avd/hw-config.c \
    android/avd/info.c \
    android/avd/scanner.c \
    android/avd/util.c \
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
    android/base/misc/HttpUtils.cpp \
    android/base/misc/StringUtils.cpp \
    android/base/misc/Utf8Utils.cpp \
    android/base/sockets/SocketDrainer.cpp \
    android/base/sockets/SocketUtils.cpp \
    android/base/sockets/SocketWaiter.cpp \
    android/base/synchronization/MessageChannel.cpp \
    android/base/Log.cpp \
    android/base/memory/LazyInstance.cpp \
    android/base/String.cpp \
    android/base/StringFormat.cpp \
    android/base/StringView.cpp \
    android/base/system/System.cpp \
    android/base/threads/ThreadStore.cpp \
    android/base/Uri.cpp \
    android/base/Version.cpp \
    android/emulation/android_pipe.c \
    android/emulation/android_pipe_pingpong.c \
    android/emulation/android_pipe_throttle.c \
    android/emulation/android_pipe_zero.c \
    android/emulation/bufprint_config_dirs.cpp \
    android/emulation/ConfigDirs.cpp \
    android/emulation/control/LineConsumer.cpp \
    android/emulation/CpuAccelerator.cpp \
    android/emulation/serial_line.cpp \
    android/kernel/kernel_utils.cpp \
    android/metrics/metrics_reporter.c \
    android/metrics/metrics_reporter_ga.c \
    android/metrics/metrics_reporter_toolbar.c \
    android/metrics/StudioHelper.cpp \
    android/opengl/EmuglBackendList.cpp \
    android/opengl/EmuglBackendScanner.cpp \
    android/opengl/emugl_config.cpp \
    android/opengl/GpuFrameBridge.cpp \
    android/proxy/proxy_common.c \
    android/proxy/proxy_http.c \
    android/proxy/proxy_http_connector.c \
    android/proxy/proxy_http_rewriter.c \
    android/update-check/UpdateChecker.cpp \
    android/update-check/VersionExtractor.cpp \
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
    android/utils/ini.c \
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
    android/utils/x86_cpuid.c \
    android/emulation/android_qemud.cpp \
    android/emulation/qemud/android_qemud_sink.cpp \
    android/emulation/qemud/android_qemud_serial.cpp \
    android/emulation/qemud/android_qemud_client.cpp \
    android/emulation/qemud/android_qemud_service.cpp \
    android/emulation/qemud/android_qemud_multiplexer.cpp \

#    android/filesystems/ext4_utils.cpp \
#    android/filesystems/fstab_parser.cpp \
#    android/filesystems/partition_types.cpp \
#    android/filesystems/ramdisk_extractor.cpp \


ifeq (windows,$(HOST_OS))
ANDROID_SKIN_SOURCES += \
    android/base/synchronization/ConditionVariable_win32.cpp \
    android/base/threads/Thread_win32.cpp \
    android/base/system/Win32Utils.cpp \
    android/utils/win32_cmdline_quote.cpp
else
ANDROID_SKIN_SOURCES += \
    android/base/threads/Thread_pthread.cpp
endif

intermediates := $(call intermediates-dir-for,$(HOST_BITS),emulator_hw_config_defs)
ANDROID_SKIN_CFLAGS += -I$(intermediates)

# support files
QEMU2_GLUE_SOURCES := \
    android-qemu2-glue/display.cpp \
    android-qemu2-glue/net-android.cpp \
    android-qemu2-glue/qemu-user-event-agent-impl.c \
    android-qemu2-glue/qemu-battery-agent-impl.c \
    android-qemu2-glue/qemu-cellular-agent-impl.c \
    android-qemu2-glue/qemu-display-agent-impl.cpp \
    android-qemu2-glue/qemu-finger-agent-impl.c \
    android-qemu2-glue/qemu-location-agent-impl.c \
    android-qemu2-glue/qemu-sensors-agent-impl.c \
    android-qemu2-glue/qemu-telephony-agent-impl.c \
    android-qemu2-glue/qemu-vm-operations-impl.c \
