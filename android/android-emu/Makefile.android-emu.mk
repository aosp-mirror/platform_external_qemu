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
#   ANDROID_EMU_BASE_INCLUDES
#       List of include paths to be used by any module that depends on
#       AndroidEmuBase
#
#   ANDROID_EMU_BASE_STATIC_LIBRARIES
#       List of static libraries to be used by any executable that depends on
#       AndroidEmuBase.
#
#   ANDROID_EMU_BASE_LDLIBS
#       List of system libraries to be used by any executable or shared library
#       that depends on AndroidEmuBase.
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

LOCAL_PATH := $(call my-dir)

_ANDROID_EMU_ROOT := $(LOCAL_PATH)

# all includes are like 'android/...', so we need to count on that
ANDROID_EMU_BASE_INCLUDES := $(_ANDROID_EMU_ROOT)
ANDROID_EMU_INCLUDES := $(ANDROID_EMU_BASE_INCLUDES) $(METRICS_PROTO_INCLUDES)

ANDROID_EMU_CFLAGS :=
ifeq ($(BUILD_TARGET_OS),linux)
    # These headers used to be included with the gcc prebuilds, but
    # now they aren't.
    # Note: make sure the system dirs take precedence.
    ANDROID_EMU_CFLAGS += -idirafter $(_ANDROID_EMU_ROOT)/../../linux-headers/
endif


###############################################################################
#
#  android-emu-base
#
#  This is a static library containing the low-level system interfaces
#  provided by android/base/ and android/utils/. It should only depend on
#  system and language runtime headers and libraries, and nothing else.
#  Everything that depends on the host implementation, e.g. Looper, shouldn't
#  be part of this library, but goes into android-emu
#

$(call start-cmake-project,android-emu-base)
LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS) $(ANDROID_EMU_CFLAGS)
LOCAL_C_INCLUDES := \
    $(EMULATOR_COMMON_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \
    $(LIBUUID_INCLUDES) \
    $(LZ4_INCLUDES) \

PRODUCED_STATIC_LIBS=android-emu-base emulator-libui
PRODUCED_PROTO_LIBS=metrics featurecontrol snapshot crashreport location emulation telephony verified-boot automation offworld

#  emulator-libui linker flags & settings
#
EMULATOR_LIBUI_LDLIBS :=
EMULATOR_LIBUI_LDFLAGS :=
EMULATOR_LIBUI_STATIC_LIBRARIES :=

EMULATOR_LIBUI_LDFLAGS += $(QT_LDFLAGS)
EMULATOR_LIBUI_LDLIBS += $(QT_LDLIBS)
EMULATOR_LIBUI_STATIC_LIBRARIES += $(ANDROID_SKIN_STATIC_LIBRARIES) $(FFMPEG_STATIC_LIBRARIES) $(LIBX264_STATIC_LIBRARIES) $(LIBVPX_STATIC_LIBRARIES) emulator-zlib
EMULATOR_LIBUI_INCLUDES += $(QT_INCLUDES) $(PROTOBUF_INCLUDES)


ifeq (linux,$(BUILD_TARGET_OS))
EMULATOR_LIBUI_LDLIBS += -lX11
endif

# gl-widget.cpp depends on libOpenGLESDispatch, which depends on
# libemugl_common. Using libOpenGLESDispatch ensures that the code
# will find and use the same host EGL/GLESv2 libraries as the ones
# used by EmuGL. Doing anything else is prone to really bad failure
# cases.
EMULATOR_LIBUI_STATIC_LIBRARIES += \
    libOpenGLESDispatch \
    libemugl_common \

# ffmpeg mac dependency
ifeq ($(BUILD_TARGET_OS),darwin)
    EMULATOR_LIBUI_LDLIBS += -lbz2
endif

$(call end-cmake-project)

# Make sure the memory modules are available. (NOP on non windows)
PROTOBUF_STATIC_LIBRARIES += $(LIBMMAN_WIN32_STATIC_LIBRARIES)

####
# Small low-level benchmark for android-emu-base.
#
$(call start-emulator-benchmark,android_emu$(BUILD_TARGET_SUFFIX)_benchmark)

LOCAL_C_INCLUDES := \
    $(ANDROID_EMU_BASE_INCLUDES) \

LOCAL_SRC_FILES := \
    android/base/synchronization/Lock_benchmark.cpp \

LOCAL_STATIC_LIBRARIES := android-emu-base

$(call end-emulator-benchmark)

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

# Common definitions

# Workaround for b/115634240
android_emu_LOCAL_SOURCE_DEPENDENCIES := $(PROTOBUF_DEPS)

android_emu_LOCAL_CFLAGS := \
    $(EMULATOR_COMMON_CFLAGS) \
    $(LIBCURL_CFLAGS) \
    $(LIBXML2_CFLAGS) \
    $(ANDROID_EMU_CFLAGS) \

android_emu_LOCAL_C_INCLUDES := \
    $(EMUGL_INCLUDES) \
    $(EMUGL_SRCDIR)/shared \
    $(EMULATOR_COMMON_INCLUDES) \
    $(EMULATOR_LIBYUV_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \
    $(EMUGL_INCLUDES) \
    $(BREAKPAD_CLIENT_INCLUDES) \
    $(LIBCURL_INCLUDES) \
    $(LIBJPEG_INCLUDES) \
    $(LIBXML2_INCLUDES) \
    $(LIBEXT4_UTILS_INCLUDES) \
    $(LIBKEYMASTER3_INCLUDES) \
    $(LIBPNG_INCLUDES) \
    $(TINYOBJLOADER_INCLUDES) \
    $(LZ4_INCLUDES) \
    $(ZLIB_INCLUDES) \
    $(MURMURHASH_INCLUDES) \
	$(PROTOBUF_INCLUDES)

android_emu_LOCAL_SRC_FILES := \
    android/adb-server.cpp \
    android/avd/generate.cpp \
    android/avd/hw-config.c \
    android/avd/info.c \
    android/avd/scanner.c \
    android/avd/util.c \
    android/avd/util_wrapper.cpp \
    android/async-console.c \
    android/async-socket.c \
    android/async-socket-connector.c \
    android/async-utils.c \
    android/base/async/AsyncReader.cpp \
    android/base/async/AsyncSocketServer.cpp \
    android/base/async/AsyncWriter.cpp \
    android/base/async/DefaultLooper.cpp \
    android/base/async/Looper.cpp \
    android/base/async/ScopedSocketWatch.cpp \
    android/base/async/ThreadLooper.cpp \
    android/base/network/Dns.cpp \
    android/base/Pool.cpp \
    android/base/sockets/SocketDrainer.cpp \
    android/base/threads/internal/ParallelTaskBase.cpp \
    android/boot-properties.c \
    android/car.cpp \
    android/cmdline-option.cpp \
    android/CommonReportedInfo.cpp \
    android/console_auth.cpp \
    android/cpu_accelerator.cpp \
    android/crashreport/CrashSystem.cpp \
    android/crashreport/CrashReporter_common.cpp \
    android/crashreport/CrashReporter_$(BUILD_TARGET_OS).cpp \
    android/crashreport/HangDetector.cpp \
    android/cros.c \
    android/curl-support.c \
    android/emuctl-client.cpp \
    android/emulation/AdbDebugPipe.cpp \
    android/emulation/AdbGuestPipe.cpp \
    android/emulation/AdbHostListener.cpp \
    android/emulation/AdbHostServer.cpp \
    android/emulation/AndroidAsyncMessagePipe.cpp \
    android/emulation/AndroidMessagePipe.cpp \
    android/emulation/AndroidPipe.cpp \
    android/emulation/android_pipe_host.cpp \
    android/emulation/AudioCaptureEngine.cpp \
    android/emulation/AudioOutputEngine.cpp \
    android/emulation/android_pipe_pingpong.c \
    android/emulation/android_pipe_throttle.c \
    android/emulation/android_pipe_unix.cpp \
    android/emulation/android_pipe_zero.c \
    android/emulation/android_qemud.cpp \
    android/emulation/bufprint_config_dirs.cpp \
    android/emulation/ClipboardPipe.cpp \
    android/emulation/ComponentVersion.cpp \
    android/emulation/ConfigDirs.cpp \
    android/emulation/control/AdbInterface.cpp \
    android/emulation/control/ApkInstaller.cpp \
    android/emulation/control/FilePusher.cpp \
    android/emulation/control/GooglePlayServices.cpp \
    android/emulation/control/LineConsumer.cpp \
    android/emulation/control/AdbBugReportServices.cpp \
    android/emulation/CpuAccelerator.cpp \
    android/emulation/DmaMap.cpp \
    android/emulation/GoldfishDma.cpp \
    android/emulation/GoldfishSyncCommandQueue.cpp \
    android/emulation/goldfish_sync.cpp \
    android/emulation/hostpipe/HostGoldfishPipe.cpp \
    android/emulation/LogcatPipe.cpp \
    android/emulation/nand_limits.c \
    android/emulation/ParameterList.cpp \
    android/emulation/qemud/android_qemud_client.cpp \
    android/emulation/qemud/android_qemud_multiplexer.cpp \
    android/emulation/qemud/android_qemud_serial.cpp \
    android/emulation/qemud/android_qemud_service.cpp \
    android/emulation/qemud/android_qemud_sink.cpp \
    android/emulation/RefcountPipe.cpp \
    android/emulation/serial_line.cpp \
    android/emulation/SerialLine.cpp \
    android/emulation/SetupParameters.cpp \
    android/emulation/testing/TestVmLock.cpp \
    android/emulation/VmLock.cpp \
    android/error-messages.cpp \
    android/featurecontrol/FeatureControl.cpp \
    android/featurecontrol/FeatureControlImpl.cpp \
    android/featurecontrol/feature_control.cpp \
    android/featurecontrol/HWMatching.cpp \
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
    android/HostHwInfo.cpp \
    android/hw-control.c \
    android/hw-events.c \
    android/hw-fingerprint.c \
    android/hw-kmsg.c \
    android/hw-lcd.c \
    android/hw-qemud.cpp \
    android/jpeg-compress.c \
    android/kernel/kernel_utils.cpp \
    android/loadpng.c \
    android/location/Point.cpp \
    android/location/Route.cpp \
    android/main-help.cpp \
    android/main-emugl.cpp \
    android/main-kernel-parameters.cpp \
    android/metrics/AdbLivenessChecker.cpp \
    android/metrics/AsyncMetricsReporter.cpp \
    android/metrics/CrashMetricsReporting.cpp \
    android/metrics/FileMetricsWriter.cpp \
    android/metrics/MemoryUsageReporter.cpp \
    android/metrics/metrics.cpp \
    android/metrics/MetricsPaths.cpp \
    android/metrics/MetricsReporter.cpp \
    android/metrics/MetricsWriter.cpp \
    android/metrics/NullMetricsReporter.cpp \
    android/metrics/NullMetricsWriter.cpp \
    android/metrics/Percentiles.cpp \
    android/metrics/PeriodicReporter.cpp \
    android/metrics/SyncMetricsReporter.cpp \
    android/metrics/StudioConfig.cpp \
    android/metrics/TextMetricsWriter.cpp \
    android/multi-instance.cpp \
    android/multitouch-port.c \
    android/multitouch-screen.c \
    android/network/control.cpp \
    android/network/constants.c \
    android/network/globals.c \
    android/network/NetworkPipe.cpp \
    android/network/wifi.cpp \
    android/opengl/EmuglBackendList.cpp \
    android/opengl/EmuglBackendScanner.cpp \
    android/opengl/emugl_config.cpp \
    android/opengl/GpuFrameBridge.cpp \
    android/opengl/GLProcessPipe.cpp \
    android/opengl/gpuinfo.cpp \
    android/opengl/logger.cpp \
    android/opengl/NativeGpuInfo_$(BUILD_TARGET_OS).cpp \
    android/opengl/OpenglEsPipe.cpp \
    android/opengles.cpp \
    android/openssl-support.cpp \
    android/process_setup.cpp \
    android/protobuf/DelimitedSerialization.cpp \
    android/protobuf/LoadSave.cpp \
    android/protobuf/ProtobufLogging.cpp \
    android/proxy/proxy_common.c \
    android/proxy/proxy_http.c \
    android/proxy/proxy_http_connector.c \
    android/proxy/proxy_http_rewriter.c \
    android/proxy/proxy_setup.cpp \
    android/proxy/ProxyUtils.cpp \
    android/qemu-tcpdump.c \
    android/qt/qt_path.cpp \
    android/qt/qt_setup.cpp \
    android/resource.c \
    android/sdk-controller-socket.c \
    android/session_phase_reporter.cpp \
    android/shaper.c \
    android/snaphost-android.c \
    android/snapshot.c \
    android/snapshot/common.cpp \
    android/snapshot/Compressor.cpp \
    android/snapshot/Decompressor.cpp \
    android/snapshot/GapTracker.cpp \
    android/snapshot/IncrementalStats.cpp \
    android/snapshot/interface.cpp \
    android/snapshot/Loader.cpp \
    android/snapshot/MemoryWatch_common.cpp \
    android/snapshot/MemoryWatch_$(BUILD_TARGET_OS).cpp \
    android/snapshot/PathUtils.cpp \
    android/snapshot/Hierarchy.cpp \
    android/snapshot/Quickboot.cpp \
    android/snapshot/RamLoader.cpp \
    android/snapshot/RamSaver.cpp \
    android/snapshot/RamSnapshotTesting.cpp \
    android/snapshot/Saver.cpp \
    android/snapshot/Snapshot.cpp \
    android/snapshot/Snapshotter.cpp \
    android/snapshot/TextureLoader.cpp \
    android/snapshot/TextureSaver.cpp \
    android/telephony/debug.c \
    android/telephony/gsm.c \
    android/telephony/modem.c \
    android/telephony/modem_driver.c \
    android/telephony/remote_call.c \
    android/telephony/phone_number.cpp \
    android/telephony/SimAccessRules.cpp \
    android/telephony/sim_card.c \
    android/telephony/sms.c \
    android/telephony/sysdeps.c \
    android/telephony/TagLengthValue.cpp \
    android/uncompress.cpp \
    android/update-check/UpdateChecker.cpp \
    android/update-check/VersionExtractor.cpp \
    android/user-config.cpp \
    android/utils/dns.cpp \
    android/utils/Random.cpp \
    android/utils/socket_drainer.cpp \
    android/utils/sockets.c \
    android/utils/looper.cpp \
    android/verified-boot/load_config.cpp \
    android/wear-agent/android_wear_agent.cpp \
    android/wear-agent/WearAgent.cpp \
    android/wear-agent/PairUpWearPhone.cpp \

# Platform-specific camera capture
ifeq ($(BUILD_TARGET_OS),linux)
    android_emu_dependent_LOCAL_SRC_FILES += \
        android/camera/camera-capture-linux.c \

endif

ifeq ($(BUILD_TARGET_OS),darwin)
    android_emu_LOCAL_SRC_FILES += \
        android/opengl/macTouchOpenGL.m \
        android/snapshot/MacSegvHandler.cpp \

    android_emu_dependent_LOCAL_SRC_FILES += \
        android/camera/camera-capture-mac.m \

endif

ifeq ($(BUILD_TARGET_OS),windows)
    android_emu_LOCAL_SRC_FILES += \
        android/windows_installer.cpp \

    android_emu_dependent_LOCAL_SRC_FILES += \
        android/camera/camera-capture-windows.cpp \

endif

# Source files in androidEmu that are dependent on other static
# libraries being there.
android_emu_dependent_LOCAL_SRC_FILES += \
    android/automation/AutomationController.cpp \
    android/automation/AutomationEventSink.cpp \
    android/camera/camera-common.cpp \
    android/camera/camera-format-converters.c \
    android/camera/camera-list.cpp \
    android/camera/camera-metrics.cpp \
    android/camera/camera-service.c \
    android/camera/camera-virtualscene.cpp \
    android/emulation/control/ScreenCapturer.cpp \
    android/emulation/FakeRotatingCameraSensor.cpp \
    android/emulation/Keymaster3.cpp \
    android/emulation/QemuMiscPipe.cpp \
    android/console.cpp \
    android/http_proxy.c \
    android/hw-sensors.cpp \
    android/main-common.c \
    android/main-qemu-parameters.cpp \
    android/offworld/OffworldPipe.cpp \
    android/physics/AmbientEnvironment.cpp \
    android/physics/InertialModel.cpp \
    android/physics/PhysicalModel.cpp \
    android/qemu-setup.cpp \
    android/sensors-port.c \
    android/snapshot/SnapshotAPI.cpp \
    android/test/checkboot.cpp \
    android/virtualscene/MeshSceneObject.cpp \
    android/virtualscene/PosterInfo.cpp \
    android/virtualscene/PosterSceneObject.cpp \
    android/virtualscene/Renderer.cpp \
    android/virtualscene/RenderTarget.cpp \
    android/virtualscene/Scene.cpp \
    android/virtualscene/SceneCamera.cpp \
    android/virtualscene/SceneObject.cpp \
    android/virtualscene/TextureUtils.cpp \
    android/virtualscene/VirtualSceneManager.cpp \
    android/virtualscene/WASDInputHandler.cpp \

# The actual invocation for the static library build.
$(call start-emulator-library,android-emu)

LOCAL_SOURCE_DEPENDENCIES += $(android_emu_LOCAL_SOURCE_DEPENDENCIES)

LOCAL_CFLAGS += $(android_emu_LOCAL_CFLAGS)

LOCAL_C_INCLUDES += $(android_emu_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES += $(android_emu_LOCAL_SRC_FILES) $(android_emu_dependent_LOCAL_SRC_FILES)

# This file can get included multiple times, with different variable
# declarations. We only want to set LOCAL_COPY_COMMON_PREBUILT_RESOURCES and
# LOCAL_COPY_COMMON_TESTDATA once. GNUmake will complain that we are overriding
# targets if we don't.
ifdef FIRST_INCLUDE
LOCAL_COPY_COMMON_PREBUILT_RESOURCES += \
    virtualscene/Toren1BD/Toren1BD.mtl \
    virtualscene/Toren1BD/Toren1BD.obj \
    virtualscene/Toren1BD/Toren1BD.posters \
    virtualscene/Toren1BD/Toren1BD_Decor.png \
    virtualscene/Toren1BD/Toren1BD_Main.png \
    virtualscene/Toren1BD/poster.png \

LOCAL_COPY_COMMON_TESTDATA += \
    snapshots/random-ram-100.bin \
    textureutils/gray_alpha_golden.bmp \
    textureutils/gray_alpha.png \
    textureutils/gray_golden.bmp \
    textureutils/gray.png \
    textureutils/indexed_alpha_golden.bmp \
    textureutils/indexed_alpha.png \
    textureutils/indexed_golden.bmp \
    textureutils/indexed.png \
    textureutils/interlaced_golden.bmp \
    textureutils/interlaced.png \
    textureutils/jpeg_gray_golden.bmp \
    textureutils/jpeg_gray.jpg \
    textureutils/jpeg_gray_progressive_golden.bmp \
    textureutils/jpeg_gray_progressive.jpg \
    textureutils/jpeg_rgb24_golden.bmp \
    textureutils/jpeg_rgb24.jpg \
    textureutils/jpeg_rgb24_progressive_golden.bmp \
    textureutils/jpeg_rgb24_progressive.jpg \
    textureutils/rgb24_31px_golden.bmp \
    textureutils/rgb24_31px.png \
    textureutils/rgba32_golden.bmp \
    textureutils/rgba32.png \

LOCAL_COPY_COMMON_TESTDATA_DIRS += \
	test-sdk \

endif

$(call gen-hw-config-defs)
$(call end-emulator-library)

# List of static libraries that anything that depends on the base libraries
# should use.
ANDROID_EMU_BASE_STATIC_LIBRARIES := \
    android-emu-base \
    $(LIBUUID_STATIC_LIBRARIES) \
    emulator-lz4 \

ANDROID_EMU_BASE_LDLIBS := \
    $(LIBUUID_LDLIBS) \

ifeq ($(BUILD_TARGET_OS),linux)
    ANDROID_EMU_BASE_LDLIBS += -lrt
    ANDROID_EMU_BASE_LDLIBS += -lX11
    ANDROID_EMU_BASE_LDLIBS += -lGL
endif
ifeq ($(BUILD_TARGET_OS),windows)
    ANDROID_EMU_BASE_LDLIBS += -lpsapi -ld3d9
endif
ifeq ($(BUILD_TARGET_OS),darwin)
    ANDROID_EMU_BASE_LDLIBS += -Wl,-framework,AppKit
    ANDROID_EMU_BASE_LDLIBS += -Wl,-framework,Accelerate
    ANDROID_EMU_BASE_LDLIBS += -Wl,-framework,IOKit
    ANDROID_EMU_BASE_LDLIBS += -Wl,-weak_framework,Hypervisor
    ANDROID_EMU_BASE_LDLIBS += -Wl,-framework,OpenGL
endif

ANDROID_EMU_STATIC_LIBRARIES_DEPS := \
    emulator-libext4_utils \
    $(ANDROID_EMU_BASE_STATIC_LIBRARIES) \
    $(LIBCURL_STATIC_LIBRARIES) \
    $(LIBXML2_STATIC_LIBRARIES) \
    $(BREAKPAD_CLIENT_STATIC_LIBRARIES) \
    emulator-libsparse \
    emulator-libselinux \
    emulator-libjpeg \
    emulator-libpng \
    emulator-libyuv \
    emulator-libwebp \
    emulator-tinyobjloader \
    emulator-zlib \
    emulator-murmurhash \
    emulator-tinyepoxy \
    $(ALL_PROTOBUF_LIBS) \
	$(PROTOBUF_STATIC_LIBRARIES) \

ANDROID_EMU_STATIC_LIBRARIES := \
    android-emu $(ANDROID_EMU_STATIC_LIBRARIES_DEPS) \
    $(LIBKEYMASTER3_STATIC_LIBRARIES) \

ANDROID_EMU_LDLIBS := \
    $(ANDROID_EMU_BASE_LDLIBS) \
    $(LIBCURL_LDLIBS) \
    $(BREAKPAD_CLIENT_LDLIBS) \

ifeq ($(BUILD_TARGET_OS),windows)
# For CoTaskMemFree used in camera-capture-windows.cpp
ANDROID_EMU_LDLIBS += -lole32
# For GetPerformanceInfo in CrashService_windows.cpp
ANDROID_EMU_LDLIBS += -lpsapi
# Winsock functions
ANDROID_EMU_LDLIBS += -lws2_32
# GetNetworkParams() for android/utils/dns.c
ANDROID_EMU_LDLIBS += -liphlpapi
endif

# Shared library variant of android-emu.
#
# Currently incomplete, but eventually we might want to use it in place
# of a static android-emu library, especially if it means we can develop
# in android-emu on Windows directly.
#
# And eventually, make android-emu both a shared library and minimize the
# interface between qemu and android-emu so that android-emu can be a plugin
# and we make minimal or only qemu-specific changes to qemu itself.
$(call start-emulator-shared-lib,android-emu-shared)

LOCAL_STATIC_LIBRARIES += $(ANDROID_EMU_STATIC_LIBRARIES_DEPS)

LOCAL_SOURCE_DEPENDENCIES += $(android_emu_LOCAL_SOURCE_DEPENDENCIES)

LOCAL_CFLAGS += $(android_emu_LOCAL_CFLAGS) -fvisibility=default

LOCAL_C_INCLUDES += $(android_emu_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES += $(android_emu_LOCAL_SRC_FILES) stubs/stubs.cpp

LOCAL_LDLIBS += $(ANDROID_EMU_LDLIBS)

$(call local-link-static-c++lib)

$(call gen-hw-config-defs)
$(call end-emulator-shared-lib)

###############################################################################
#
#  android-emu unit tests
#
#

$(call start-emulator-program, android_emu$(BUILD_TARGET_SUFFIX)_unittests)
$(call gen-hw-config-defs)

# Workaround for b/115634240
LOCAL_SOURCE_DEPENDENCIES := $(PROTOBUF_DEPS)

LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(EMULATOR_COMMON_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \
    $(LIBXML2_INCLUDES) \
    $(EMUGL_INCLUDES) \
    $(LZ4_INCLUDES) \
	$(PROTOBUF_INCLUDES)

LOCAL_LDLIBS += \
    $(ANDROID_EMU_LDLIBS) \

LOCAL_SRC_FILES := \
  android/automation/AutomationController_unittest.cpp \
  android/automation/AutomationEventSink_unittest.cpp \
  android/avd/util_unittest.cpp \
  android/avd/util_wrapper_unittest.cpp \
  android/base/ArraySize_unittest.cpp \
  android/base/AlignedBuf_unittest.cpp \
  android/base/ContiguousRangeMapper_unittest.cpp \
  android/base/async/Looper_unittest.cpp \
  android/base/async/AsyncSocketServer_unittest.cpp \
  android/base/async/RecurrentTask_unittest.cpp \
  android/base/async/ScopedSocketWatch_unittest.cpp \
  android/base/async/SubscriberList_unittest.cpp \
  android/base/containers/BufferQueue_unittest.cpp \
  android/base/containers/CircularBuffer_unittest.cpp \
  android/base/containers/Lookup_unittest.cpp \
  android/base/containers/SmallVector_unittest.cpp \
  android/base/containers/StaticMap_unittest.cpp \
  android/base/EintrWrapper_unittest.cpp \
  android/base/files/FileShareOpen_unittest.cpp \
  android/base/files/IniFile_unittest.cpp \
  android/base/files/InplaceStream_unittest.cpp \
  android/base/files/MemStream_unittest.cpp \
  android/base/files/PathUtils_unittest.cpp \
  android/base/files/ScopedFd_unittest.cpp \
  android/base/files/ScopedStdioFile_unittest.cpp \
  android/base/files/Stream_unittest.cpp \
  android/base/files/StreamSerializing_unittest.cpp \
  android/base/FunctionView_unittest.cpp \
  android/base/Log_unittest.cpp \
  android/base/memory/LazyInstance_unittest.cpp \
  android/base/memory/MemoryHints_unittest.cpp \
  android/base/memory/MallocUsableSize_unittest.cpp \
  android/base/memory/OnDemand_unittest.cpp \
  android/base/memory/ScopedPtr_unittest.cpp \
  android/base/memory/SharedMemory_unittest.cpp \
  android/base/misc/FileUtils_unittest.cpp \
  android/base/misc/HttpUtils_unittest.cpp \
  android/base/misc/StringUtils_unittest.cpp \
  android/base/misc/Utf8Utils_unittest.cpp \
  android/base/network/Dns_unittest.cpp \
  android/base/network/IpAddress_unittest.cpp \
  android/base/network/NetworkUtils_unittest.cpp \
  android/base/Optional_unittest.cpp \
  android/base/Pool_unittest.cpp \
  android/base/ProcessControl_unittest.cpp \
  android/base/Result_unittest.cpp \
  android/base/sockets/ScopedSocket_unittest.cpp \
  android/base/sockets/SocketDrainer_unittest.cpp \
  android/base/sockets/SocketUtils_unittest.cpp \
  android/base/sockets/SocketWaiter_unittest.cpp \
  android/base/StringFormat_unittest.cpp \
  android/base/StringParse_unittest.cpp \
  android/base/StringView_unittest.cpp \
  android/base/synchronization/ConditionVariable_unittest.cpp \
  android/base/synchronization/Lock_unittest.cpp \
  android/base/synchronization/ReadWriteLock_unittest.cpp \
  android/base/synchronization/MessageChannel_unittest.cpp \
  android/base/system/System_unittest.cpp \
  android/base/testing/MockUtils_unittest.cpp \
  android/base/testing/ProtobufMatchers.cpp \
  android/base/testing/TestEvent_unittest.cpp \
  android/base/threads/Async_unittest.cpp \
  android/base/threads/FunctorThread_unittest.cpp \
  android/base/threads/ParallelTask_unittest.cpp \
  android/base/threads/Thread_unittest.cpp \
  android/base/threads/ThreadStore_unittest.cpp \
  android/base/TypeTraits_unittest.cpp \
  android/base/Uri_unittest.cpp \
  android/base/Uuid_unittest.cpp \
  android/base/Version_unittest.cpp \
  android/camera/CameraFormatConverters_unittest.cpp \
  android/cmdline-option_unittest.cpp \
  android/CommonReportedInfo_unittest.cpp \
  android/console_auth_unittest.cpp \
  android/console_unittest.cpp \
  android/emulation/AdbDebugPipe_unittest.cpp \
  android/emulation/AdbGuestPipe_unittest.cpp \
  android/emulation/AdbHostListener_unittest.cpp \
  android/emulation/AdbHostServer_unittest.cpp \
  android/emulation/android_pipe_pingpong_unittest.cpp \
  android/emulation/android_pipe_zero_unittest.cpp \
  android/emulation/AndroidAsyncMessagePipe_unittest.cpp \
  android/emulation/bufprint_config_dirs_unittest.cpp \
  android/emulation/ComponentVersion_unittest.cpp \
  android/emulation/ConfigDirs_unittest.cpp \
  android/emulation/DeviceContextRunner_unittest.cpp \
  android/emulation/DmaMap_unittest.cpp \
  android/emulation/control/AdbInterface_unittest.cpp \
  android/emulation/control/ApkInstaller_unittest.cpp \
  android/emulation/control/FilePusher_unittest.cpp \
  android/emulation/control/GooglePlayServices_unittest.cpp \
  android/emulation/control/ScreenCapturer_unittest.cpp \
  android/emulation/control/LineConsumer_unittest.cpp \
  android/emulation/CpuAccelerator_unittest.cpp \
  android/emulation/Hypervisor_unittest.cpp \
  android/emulation/hostpipe/HostGoldfishPipe_unittest.cpp \
  android/emulation/ParameterList_unittest.cpp \
  android/emulation/RefcountPipe_unittest.cpp \
  android/emulation/serial_line_unittest.cpp \
  android/emulation/SetupParameters_unittest.cpp \
  android/emulation/testing/TestAndroidPipeDevice.cpp \
  android/emulation/testing/MockAndroidEmulatorWindowAgent.cpp \
  android/emulation/testing/MockAndroidSensorsAgent.cpp \
  android/emulation/testing/MockAndroidVmOperations.cpp \
  android/emulation/VmLock_unittest.cpp \
  android/error-messages_unittest.cpp \
  android/featurecontrol/FeatureControl_unittest.cpp \
  android/featurecontrol/HWMatching_unittest.cpp \
  android/featurecontrol/testing/FeatureControlTest.cpp \
  android/filesystems/ext4_resize_unittest.cpp \
  android/filesystems/ext4_utils_unittest.cpp \
  android/filesystems/fstab_parser_unittest.cpp \
  android/filesystems/partition_config_unittest.cpp \
  android/filesystems/partition_types_unittest.cpp \
  android/filesystems/ramdisk_extractor_unittest.cpp \
  android/filesystems/testing/TestSupport.cpp \
  android/gps/GpxParser_unittest.cpp \
  android/gps/KmlParser_unittest.cpp \
  android/hw-lcd_unittest.cpp \
  android/kernel/kernel_utils_unittest.cpp \
  android/location/Point_unittest.cpp \
  android/location/Route_unittest.cpp \
  android/network/control_unittest.cpp \
  android/network/constants_unittest.cpp \
  android/offworld/OffworldPipe_unittest.cpp \
  android/opengl/EmuglBackendList_unittest.cpp \
  android/opengl/EmuglBackendScanner_unittest.cpp \
  android/opengl/emugl_config_unittest.cpp \
  android/opengl/GpuFrameBridge_unittest.cpp \
  android/opengl/gpuinfo_unittest.cpp \
  android/physics/AmbientEnvironment_unittest.cpp \
  android/physics/InertialModel_unittest.cpp \
  android/physics/PhysicalModel_unittest.cpp \
  android/proxy/proxy_common_unittest.cpp \
  android/proxy/ProxyUtils_unittest.cpp \
  android/qt/qt_path_unittest.cpp \
  android/qt/qt_setup_unittest.cpp \
  android/snapshot/RamLoader_unittest.cpp \
  android/snapshot/RamSaver_unittest.cpp \
  android/snapshot/RamSnapshot_unittest.cpp \
  android/snapshot/Snapshot_unittest.cpp \
  android/telephony/gsm_unittest.cpp \
  android/telephony/modem_unittest.cpp \
  android/telephony/sms_unittest.cpp \
  android/telephony/SimAccessRules_unittest.cpp \
  android/telephony/TagLengthValue_unittest.cpp \
  android/update-check/UpdateChecker_unittest.cpp \
  android/update-check/VersionExtractor_unittest.cpp \
  android/utils/aconfig-file_unittest.cpp \
  android/utils/bufprint_unittest.cpp \
  android/utils/dirscanner_unittest.cpp \
  android/utils/dns_unittest.cpp \
  android/utils/eintr_wrapper_unittest.cpp \
  android/utils/file_data_unittest.cpp \
  android/utils/filelock_unittest.cpp \
  android/utils/format_unittest.cpp \
  android/utils/host_bitness_unittest.cpp \
  android/utils/path_unittest.cpp \
  android/utils/property_file_unittest.cpp \
  android/utils/Random_unittest.cpp \
  android/utils/string_unittest.cpp \
  android/utils/sockets_unittest.cpp \
  android/utils/x86_cpuid_unittest.cpp \
  android/verified-boot/load_config_unittest.cpp \
  android/virtualscene/TextureUtils_unittest.cpp \
  android/virtualscene/WASDInputHandler_unittest.cpp \
  android/wear-agent/PairUpWearPhone_unittest.cpp \
  android/wear-agent/testing/WearAgentTestUtils.cpp \
  android/wear-agent/WearAgent_unittest.cpp \

ifeq (windows,$(BUILD_TARGET_OS))
LOCAL_SRC_FILES += \
  android/base/files/ScopedFileHandle_unittest.cpp \
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
    $(EMULATOR_GTEST_STATIC_LIBRARIES) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from
# $(BUILD_OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)

###############################################################################
#
#  android-emu-metrics unit tests
#
#

$(call start-emulator-program, \
    android_emu_metrics$(BUILD_TARGET_SUFFIX)_unittests)

# Workaround for b/115634240
LOCAL_SOURCE_DEPENDENCIES := $(PROTOBUF_DEPS)

LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(EMULATOR_COMMON_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \
    $(LIBXML2_INCLUDES) \
	$(PROTOBUF_INCLUDES)

LOCAL_LDLIBS += \
    $(ANDROID_EMU_LDLIBS) \

LOCAL_SRC_FILES := \
  android/metrics/StudioConfig_unittest.cpp \
  android/metrics/tests/AsyncMetricsReporter_unittest.cpp \
  android/metrics/tests/FileMetricsWriter_unittest.cpp \
  android/metrics/tests/MetricsReporter_unittest.cpp \
  android/metrics/tests/MockMetricsReporter.cpp \
  android/metrics/tests/MockMetricsWriter.cpp \
  android/metrics/tests/NullMetricsClasses_unittest.cpp \
  android/metrics/tests/Percentiles_unittest.cpp \
  android/metrics/tests/PeriodicReporter_unittest.cpp \
  android/metrics/tests/SyncMetricsReporter_unittest.cpp \

LOCAL_CFLAGS += -O0

LOCAL_STATIC_LIBRARIES += \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    $(EMULATOR_GTEST_STATIC_LIBRARIES) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from
# $(BUILD_OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)



# emulator-libui unit tests


$(call start-emulator-program, emulator$(BUILD_TARGET_SUFFIX)_libui_unittests)

LOCAL_C_INCLUDES += \
    $(EMULATOR_COMMON_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \
    $(FFMPEG_INCLUDES) \

# Recompile FfmpegRecorder.cpp so we can keep assertions enabled for death tests
LOCAL_SRC_FILES := \
    android/skin/keycode_unittest.cpp \
    android/skin/keycode-buffer_unittest.cpp \
    android/skin/rect_unittest.cpp \
    android/recording/test/DummyAudioProducer.cpp \
    android/recording/test/DummyVideoProducer.cpp \
    android/recording/FfmpegRecorder.cpp \
    android/recording/test/FfmpegRecorder_unittest.cpp \

# ffmpeg targets C, so it doesn't care that C++11 requres a space bewteen
# string literals which are being glued together
LOCAL_CXXFLAGS += $(call if-target-clang,-Wno-reserved-user-defined-literal,-Wno-literal-suffix)

LOCAL_LDLIBS := $(ANDROID_EMU_LDLIBS)
# ffmpeg mac dependency
ifeq ($(BUILD_TARGET_OS),darwin)
    LOCAL_LDLIBS += -lbz2
endif

LOCAL_C_INCLUDES += \
    $(LIBXML2_INCLUDES) \

LOCAL_CFLAGS += -O0 -UNDEBUG
LOCAL_STATIC_LIBRARIES += \
    emulator-libui \
    $(EMULATOR_GTEST_STATIC_LIBRARIES) \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    $(FFMPEG_STATIC_LIBRARIES) \
    $(LIBX264_STATIC_LIBRARIES) \
    $(LIBVPX_STATIC_LIBRARIES) \
    emulator-zlib \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from
# $(BUILD_OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)
$(call gen-hw-config-defs)

$(call end-emulator-program)

# Done

LOCAL_PATH := $(_ANDROID_EMU_OLD_LOCAL_PATH)
