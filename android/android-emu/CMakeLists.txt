cmake_minimum_required(VERSION 3.5)
project(base-libraries)


# COMPILER + PATH CONFIGURATION
# =============================
# TODO(jansene): This section is needed as a bridge
# to the android build system.
string(REPLACE " " ";" INCLUDES ${LOCAL_C_INCLUDES})
include_directories(${INCLUDES})
include_directories(.)

# Use same compiler config as android build system
set(CMAKE_C_FLAGS ${LOCAL_CFLAGS})
set(CMAKE_CXX_FLAGS  "${LOCAL_CFLAGS} ${LOCAL_CXXFLAGS}")

# Work around some gcc/mingw issues
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Make sure we create archives in the same way..
SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")

# END COMPILER CONFIGURATION
# ==========================

set(COMMON_EMUBASE_SOURCES
    android/base/ContiguousRangeMapper.cpp
    android/base/Debug.cpp
    android/base/files/CompressingStream.cpp
    android/base/files/DecompressingStream.cpp
    android/base/files/Fd.cpp
    android/base/files/FileShareOpen.cpp
    android/base/files/IniFile.cpp
    android/base/files/InplaceStream.cpp
    android/base/files/MemStream.cpp
    android/base/files/PathUtils.cpp
    android/base/files/StdioStream.cpp
    android/base/files/Stream.cpp
    android/base/files/StreamSerializing.cpp
    android/base/misc/FileUtils.cpp
    android/base/misc/HttpUtils.cpp
    android/base/misc/StringUtils.cpp
    android/base/misc/Utf8Utils.cpp
    android/base/network/IpAddress.cpp
    android/base/network/NetworkUtils.cpp
    android/base/Stopwatch.cpp
    android/base/StringFormat.cpp
    android/base/StringParse.cpp
    android/base/StringView.cpp
    android/base/sockets/SocketUtils.cpp
    android/base/sockets/SocketWaiter.cpp
    android/base/synchronization/MessageChannel.cpp
    android/base/Log.cpp
    android/base/memory/LazyInstance.cpp
    android/base/memory/MemoryHints.cpp
    android/base/ProcessControl.cpp
    android/base/system/System.cpp
    android/base/threads/Async.cpp
    android/base/threads/FunctorThread.cpp
    android/base/threads/ThreadStore.cpp
    android/base/Uri.cpp
    android/base/Uuid.cpp
    android/base/Version.cpp
    android/utils/aconfig-file.c
    android/utils/assert.c
    android/utils/async.cpp
    android/utils/bufprint.c
    android/utils/bufprint_system.cpp
    android/utils/cbuffer.c
    android/utils/debug.c
    android/utils/debug_wrapper.cpp
    android/utils/dll.c
    android/utils/dirscanner.cpp
    android/utils/eintr_wrapper.c
    android/utils/exec.cpp
    android/utils/fd.cpp
    android/utils/filelock.cpp
    android/utils/file_data.c
    android/utils/file_io.cpp
    android/utils/format.cpp
    android/utils/host_bitness.cpp
    android/utils/http_utils.cpp
    android/utils/iolooper.cpp
    android/utils/ini.cpp
    android/utils/intmap.c
    android/utils/ipaddr.cpp
    android/utils/lineinput.c
    android/utils/lock.cpp
    android/utils/mapfile.c
    android/utils/misc.c
    android/utils/panic.c
    android/utils/path.cpp
    android/utils/path_system.cpp
    android/utils/property_file.c
    android/utils/reflist.c
    android/utils/refset.c
    android/utils/sockets.c
    android/utils/stralloc.c
    android/utils/stream.cpp
    android/utils/string.cpp
    android/utils/system.c
    android/utils/system_wrapper.cpp
    android/utils/tempfile.c
    android/utils/timezone.cpp
    android/utils/uri.cpp
    android/utils/utf8_utils.cpp
    android/utils/vector.c
    android/utils/x86_cpuid.cpp
    )

# WIN32 Tag does not work on cross compilation, but will
# work when you are active on windows
if(WIN32 OR ("${LOCAL_OS}" STREQUAL "windows") )
    set(PLATFORM_EMUBASE_SOURCES
        android/base/files/preadwrite.cpp
        android/base/memory/SharedMemory_win32.cpp
        android/base/threads/Thread_win32.cpp
        android/base/system/Win32Utils.cpp
        android/base/system/Win32UnicodeString.cpp
        android/utils/win32_cmdline_quote.cpp
        android/utils/win32_unicode.cpp
        )
else()
   set(PLATFORM_EMUBASE_SOURCES
       android/base/memory/SharedMemory_posix.cpp
       android/base/threads/Thread_pthread.cpp
       )
endif()

if(APPLE)
    set (PLATFORM_EMUBASE_SOURCES
        ${PLATFORM_EMUBASE_SOURCES}
        android/base/system/system-native-mac.mm)
endif(APPLE)

add_library(android-emu-base ${COMMON_EMUBASE_SOURCES} ${PLATFORM_EMUBASE_SOURCES})


set(COMMON_EMU_SOURCES
    android/adb-server.cpp
    android/automation/AutomationController.cpp
    android/automation/AutomationEventSink.cpp
    android/avd/generate.cpp
    android/avd/hw-config.c
    android/avd/info.c
    android/avd/scanner.c
    android/avd/util.c
    android/avd/util_wrapper.cpp
    android/async-console.c
    android/async-socket.c
    android/async-socket-connector.c
    android/async-utils.c
    android/base/async/AsyncReader.cpp
    android/base/async/AsyncSocketServer.cpp
    android/base/async/AsyncWriter.cpp
    android/base/async/DefaultLooper.cpp
    android/base/async/Looper.cpp
    android/base/async/ScopedSocketWatch.cpp
    android/base/async/ThreadLooper.cpp
    android/base/network/Dns.cpp
    android/base/Pool.cpp
    android/base/sockets/SocketDrainer.cpp
    android/base/threads/internal/ParallelTaskBase.cpp
    android/boot-properties.c
    android/camera/camera-common.cpp
    android/camera/camera-format-converters.c
    android/camera/camera-list.cpp
    android/camera/camera-metrics.cpp
    android/camera/camera-service.c
    android/camera/camera-virtualscene.cpp
    android/car.cpp
    android/cmdline-option.cpp
    android/CommonReportedInfo.cpp
    android/console.cpp
    android/console_auth.cpp
    android/cpu_accelerator.cpp
    android/crashreport/CrashSystem.cpp
    android/crashreport/CrashReporter_common.cpp
    android/crashreport/HangDetector.cpp
    android/cros.c
    android/curl-support.c
    android/emuctl-client.cpp
    android/emulation/AdbDebugPipe.cpp
    android/emulation/AdbGuestPipe.cpp
    android/emulation/AdbHostListener.cpp
    android/emulation/AdbHostServer.cpp
    android/emulation/AndroidMessagePipe.cpp
    android/emulation/AndroidPipe.cpp
    android/emulation/android_pipe_host.cpp
    android/emulation/AudioCaptureEngine.cpp
    android/emulation/AudioOutputEngine.cpp
    android/emulation/android_pipe_pingpong.c
    android/emulation/android_pipe_throttle.c
    android/emulation/android_pipe_unix.cpp
    android/emulation/android_pipe_zero.c
    android/emulation/android_qemud.cpp
    android/emulation/bufprint_config_dirs.cpp
    android/emulation/ClipboardPipe.cpp
    android/emulation/ComponentVersion.cpp
    android/emulation/ConfigDirs.cpp
    android/emulation/control/AdbInterface.cpp
    android/emulation/control/ApkInstaller.cpp
    android/emulation/control/FilePusher.cpp
    android/emulation/control/GooglePlayServices.cpp
    android/emulation/control/LineConsumer.cpp
    android/emulation/control/ScreenCapturer.cpp
    android/emulation/control/AdbBugReportServices.cpp
    android/emulation/CpuAccelerator.cpp
    android/emulation/DmaMap.cpp
    android/emulation/GoldfishDma.cpp
    android/emulation/GoldfishSyncCommandQueue.cpp
    android/emulation/goldfish_sync.cpp
    android/emulation/LogcatPipe.cpp
    android/emulation/Keymaster3.cpp
    android/emulation/FakeRotatingCameraSensor.cpp
    android/emulation/QemuMiscPipe.cpp
    android/emulation/nand_limits.c
    android/emulation/ParameterList.cpp
    android/emulation/qemud/android_qemud_client.cpp
    android/emulation/qemud/android_qemud_multiplexer.cpp
    android/emulation/qemud/android_qemud_serial.cpp
    android/emulation/qemud/android_qemud_service.cpp
    android/emulation/qemud/android_qemud_sink.cpp
    android/emulation/serial_line.cpp
    android/emulation/SerialLine.cpp
    android/emulation/SetupParameters.cpp
    android/emulation/VmLock.cpp
    android/error-messages.cpp
    android/featurecontrol/FeatureControl.cpp
    android/featurecontrol/FeatureControlImpl.cpp
    android/featurecontrol/feature_control.cpp
    android/featurecontrol/HWMatching.cpp
    android/filesystems/ext4_resize.cpp
    android/filesystems/ext4_utils.cpp
    android/filesystems/fstab_parser.cpp
    android/filesystems/internal/PartitionConfigBackend.cpp
    android/filesystems/partition_config.cpp
    android/filesystems/partition_types.cpp
    android/filesystems/ramdisk_extractor.cpp
    android/framebuffer.c
    android/gps/GpxParser.cpp
    android/gps/KmlParser.cpp
    android/gps.c
    android/gpu_frame.cpp
    android/help.c
    android/http_proxy.c
    android/HostHwInfo.cpp
    android/hw-control.c
    android/hw-events.c
    android/hw-fingerprint.c
    android/hw-kmsg.c
    android/hw-lcd.c
    android/hw-qemud.cpp
    android/hw-sensors.cpp
    android/jpeg-compress.c
    android/kernel/kernel_utils.cpp
    android/loadpng.c
    android/location/Point.cpp
    android/location/Route.cpp
    android/main-common.c
    android/main-help.cpp
    android/main-emugl.cpp
    android/main-kernel-parameters.cpp
    android/main-qemu-parameters.cpp
    android/metrics/AdbLivenessChecker.cpp
    android/metrics/AsyncMetricsReporter.cpp
    android/metrics/CrashMetricsReporting.cpp
    android/metrics/FileMetricsWriter.cpp
    android/metrics/MemoryUsageReporter.cpp
    android/metrics/metrics.cpp
    android/metrics/MetricsPaths.cpp
    android/metrics/MetricsReporter.cpp
    android/metrics/MetricsWriter.cpp
    android/metrics/NullMetricsReporter.cpp
    android/metrics/NullMetricsWriter.cpp
    android/metrics/Percentiles.cpp
    android/metrics/PeriodicReporter.cpp
    android/metrics/SyncMetricsReporter.cpp
    android/metrics/StudioConfig.cpp
    android/metrics/TextMetricsWriter.cpp
    android/multi-instance.cpp
    android/multitouch-port.c
    android/multitouch-screen.c
    android/network/control.cpp
    android/network/constants.c
    android/network/globals.c
    android/opengl/EmuglBackendList.cpp
    android/opengl/EmuglBackendScanner.cpp
    android/opengl/emugl_config.cpp
    android/opengl/GpuFrameBridge.cpp
    android/opengl/GLProcessPipe.cpp
    android/opengl/gpuinfo.cpp
    android/opengl/logger.cpp
    android/opengl/OpenglEsPipe.cpp
    android/opengles.cpp
    android/openssl-support.cpp
    android/physics/AmbientEnvironment.cpp
    android/physics/InertialModel.cpp
    android/physics/PhysicalModel.cpp
    android/process_setup.cpp
    android/protobuf/DelimitedSerialization.cpp
    android/protobuf/LoadSave.cpp
    android/protobuf/ProtobufLogging.cpp
    android/proxy/proxy_common.c
    android/proxy/proxy_http.c
    android/proxy/proxy_http_connector.c
    android/proxy/proxy_http_rewriter.c
    android/proxy/proxy_setup.cpp
    android/proxy/ProxyUtils.cpp
    android/qemu-setup.cpp
    android/qemu-tcpdump.c
    android/qt/qt_path.cpp
    android/qt/qt_setup.cpp
    android/resource.c
    android/sdk-controller-socket.c
    android/sensors-port.c
    android/session_phase_reporter.cpp
    android/shaper.c
    android/snaphost-android.c
    android/snapshot.c
    android/snapshot/common.cpp
    android/snapshot/Compressor.cpp
    android/snapshot/Decompressor.cpp
    android/snapshot/GapTracker.cpp
    android/snapshot/IncrementalStats.cpp
    android/snapshot/interface.cpp
    android/snapshot/Loader.cpp
    android/snapshot/MemoryWatch_common.cpp
    android/snapshot/PathUtils.cpp
    android/snapshot/Hierarchy.cpp
    android/snapshot/Quickboot.cpp
    android/snapshot/RamLoader.cpp
    android/snapshot/RamSaver.cpp
    android/snapshot/RamSnapshotTesting.cpp
    android/snapshot/Saver.cpp
    android/snapshot/Snapshot.cpp
    android/snapshot/SnapshotPipe.cpp
    android/snapshot/Snapshotter.cpp
    android/snapshot/TextureLoader.cpp
    android/snapshot/TextureSaver.cpp
    android/telephony/debug.c
    android/telephony/gsm.c
    android/telephony/modem.c
    android/telephony/modem_driver.c
    android/telephony/remote_call.c
    android/telephony/phone_number.cpp
    android/telephony/SimAccessRules.cpp
    android/telephony/sim_card.c
    android/telephony/sms.c
    android/telephony/sysdeps.c
    android/telephony/TagLengthValue.cpp
    android/test/checkboot.cpp
    android/uncompress.cpp
    android/update-check/UpdateChecker.cpp
    android/update-check/VersionExtractor.cpp
    android/user-config.cpp
    android/utils/dns.cpp
    android/utils/Random.cpp
    android/utils/socket_drainer.cpp
    android/utils/sockets.c
    android/utils/looper.cpp
    android/verified-boot/load_config.cpp
    android/virtualscene/MeshSceneObject.cpp
    android/virtualscene/PosterInfo.cpp
    android/virtualscene/PosterSceneObject.cpp
    android/virtualscene/Renderer.cpp
    android/virtualscene/RenderTarget.cpp
    android/virtualscene/Scene.cpp
    android/virtualscene/SceneCamera.cpp
    android/virtualscene/SceneObject.cpp
    android/virtualscene/TextureUtils.cpp
    android/virtualscene/VirtualSceneManager.cpp
    android/wear-agent/android_wear_agent.cpp
    android/wear-agent/WearAgent.cpp
    android/wear-agent/PairUpWearPhone.cpp
    )

if(WIN32 OR ("${LOCAL_OS}" STREQUAL "windows") )
    set(PLATFORM_EMU_SOURCES
        android/camera/camera-capture-windows.cpp
        android/crashreport/CrashReporter_windows.cpp
        android/opengl/NativeGpuInfo_windows.cpp
        android/snapshot/MemoryWatch_windows.cpp
        android/windows_installer.cpp
        )
elseif(APPLE)
    set (PLATFORM_EMU_SOURCES
        ${PLATFORM_EMU_SOURCES}
        android/camera/camera-capture-darwin.cpp
        android/camera/camera-capture-mac.m
        android/opengl/NativeGpuInfo_darwin.cpp
        android/opengl/macTouchOpenGL.m
        android/snapshot/MacSegvHandler.cpp
        android/snapshot/MemoryWatch_darwin.cpp
    )
elseif (UNIX AND NOT APPLE)
    set (PLATFORM_EMU_SOURCES
        ${PLATFORM_EMU_SOURCES}
        android/camera/camera-capture-linux.c
        android/crashreport/CrashReporter_linux.cpp
        android/opengl/NativeGpuInfo_linux.cpp
        android/snapshot/MemoryWatch_linux.cpp
        )
endif()

add_library(android-emu ${COMMON_EMU_SOURCES} ${PLATFORM_EMU_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})
