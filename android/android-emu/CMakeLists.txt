cmake_minimum_required(VERSION 3.5)
project(base-libraries)


# COMPILER + PATH CONFIGURATION
# =============================
# TODO(jansene): This section is needed as a bridge
# to the android build system.
string(REPLACE " " ";" INCLUDES ${LOCAL_C_INCLUDES})
include_directories(${INCLUDES})

# Please do not try to test compile an executable..
# We cannot run .exe on linux
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

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

## QT Settings ##


# TODO(jansene): Combine this with FindQt5 somehow as all our
# Ie. find_package(Qt5 COMPONENTS Core Widgets Gui Svg REQUIRED)

# prebuilts are Lin/Mac only..
if(NOT TARGET Qt5::moc)
    set(ENV{LD_LIBRARY_PATH} "${LOCAL_QT5}/lib:$ENV{LD_LIBRARY_PATH}")
    set(QT_VERSION_MAJOR 5)
    set(CMAKE_AUTOMOC TRUE)
    set(CMAKE_AUTOUIC TRUE)
    set(CMAKE_AUTORCC TRUE)
    set(QT_MOC_EXECUTABLE ${LOCAL_QT5}/bin/moc)
    set(QT_UIC_EXECUTABLE ${LOCAL_QT5}/bin/uic)
    set(QT_RCC_EXECUTABLE ${LOCAL_QT5}/bin/rcc)
    add_executable(Qt5::moc IMPORTED GLOBAL)
    set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION  ${QT_MOC_EXECUTABLE})
    add_executable(Qt5::uic IMPORTED GLOBAL)
    set_property(TARGET Qt5::uic PROPERTY IMPORTED_LOCATION  ${QT_UIC_EXECUTABLE})
    add_executable(Qt5::rcc IMPORTED GLOBAL)
    set_property(TARGET Qt5::rcc PROPERTY IMPORTED_LOCATION  ${QT_RCC_EXECUTABLE})
endif()
set(QT_INCLUDE_DIR "${LOCAL_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/common/include/")
set(QT_TARGET_INCLUDE_DIR "${LOCAL_QT5}/include.system")

# hw-config generator
add_custom_command(PRE_BUILD
                   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                   COMMAND python ${LOCAL_QEMU2_TOP_DIR}/android/scripts/gen-hw-config.py
                                  ${LOCAL_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini
                                  ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                   VERBATIM )
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES GENERATED TRUE)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES HEADER_FILE_ONLY TRUE)
set(GENERATED_SOURCES       ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h)
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

# Generator dependencies..
set_source_files_properties(${COMMON_EMUBASE_SOURCES} ${PLATFORM_EMUBASE_SOURCES} PROPERTIES OBJECT_DEPENDS ${GENERATED_SOURCES})
add_library(android-emu-base ${COMMON_EMUBASE_SOURCES} ${PLATFORM_EMUBASE_SOURCES})
target_include_directories(android-emu-base PRIVATE ${CMAKE_CURRENT_BINARY_DIR})


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
        android/camera/camera-capture-mac.m
        android/crashreport/CrashReporter_darwin.cpp
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

set_source_files_properties(${PLATFORM_EMU_SOURCES} PROPERTIES OBJECT_DEPENDS ${GENERATED_SOURCES})
set_source_files_properties(${COMMON_EMU_SOURCES}   PROPERTIES OBJECT_DEPENDS ${GENERATED_SOURCES})
add_library(android-emu ${COMMON_EMU_SOURCES} ${PLATFORM_EMU_SOURCES})
target_include_directories(android-emu PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(android-emu PRIVATE ../third_party/jpeg-6b)
target_include_directories(android-emu PRIVATE ../third_party/murmurhash)
target_include_directories(android-emu PRIVATE ../../../tinyobjloader)


set(ANDROID_LIBUI_SRC_FILES
    android/skin/charmap.c
    android/skin/rect.c
    android/skin/generic-event.cpp
    android/skin/generic-event-buffer.cpp
    android/skin/image.c
    android/skin/trackball.c
    android/skin/keyboard.c
    android/skin/keycode.c
    android/skin/keycode-buffer.c
    android/skin/file.c
    android/skin/window.c
    android/skin/resource.c
    android/skin/ui.c
    android/skin/qt/event-qt.cpp
    android/skin/qt/init-qt.cpp
    android/skin/qt/surface-qt.cpp
    android/skin/qt/winsys-qt.cpp
    android/skin/qt/accelerometer-3d-widget.cpp
    android/skin/qt/angle-input-widget.cpp
    android/skin/qt/editable-slider-widget.cpp
    android/skin/qt/emulator-container.cpp
    android/skin/qt/emulator-overlay.cpp
    android/skin/qt/emulator-qt-window.cpp
    android/skin/qt/emulator-qt-no-window.cpp
    android/skin/qt/event-capturer.cpp
    android/skin/qt/event-serializer.cpp
    android/skin/qt/event-subscriber.cpp
    android/skin/qt/error-dialog.cpp
    android/skin/qt/gl-canvas.cpp
    android/skin/qt/gl-common.cpp
    android/skin/qt/gl-texture-draw.cpp
    android/skin/qt/gl-widget.cpp
    android/skin/qt/extended-pages/common.cpp
    android/skin/qt/extended-pages/battery-page.cpp
    android/skin/qt/extended-pages/bug-report-page.cpp
    android/skin/qt/extended-pages/camera-page.cpp
    android/skin/qt/extended-pages/camera-virtualscene-subpage.cpp
    android/skin/qt/extended-pages/car-data-page.cpp
    android/skin/qt/extended-pages/cellular-page.cpp
    android/skin/qt/extended-pages/dpad-page.cpp
    android/skin/qt/extended-pages/finger-page.cpp
    android/skin/qt/extended-pages/google-play-page.cpp
    android/skin/qt/extended-pages/help-page.cpp
    android/skin/qt/extended-pages/location-page.cpp
    android/skin/qt/extended-pages/microphone-page.cpp
    android/skin/qt/extended-pages/record-screen-page.cpp
    android/skin/qt/extended-pages/rotary-input-dial.cpp
    android/skin/qt/extended-pages/rotary-input-page.cpp
    android/skin/qt/extended-pages/settings-page-proxy.cpp
    android/skin/qt/extended-pages/settings-page.cpp
    android/skin/qt/extended-pages/snapshot-page.cpp
    android/skin/qt/extended-pages/telephony-page.cpp
    android/skin/qt/extended-pages/virtual-sensors-page.cpp
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.cpp
    android/skin/qt/extended-window.cpp
    android/skin/qt/FramelessDetector.cpp
    android/skin/qt/ModalOverlay.cpp
    android/skin/qt/OverlayMessageCenter.cpp
    android/skin/qt/poster-image-well.cpp
    android/skin/qt/qt-ui-commands.cpp
    android/skin/qt/QtLooper.cpp
    android/skin/qt/QtThreading.cpp
    android/skin/qt/screen-mask.cpp
    android/skin/qt/size-tweaker.cpp
    android/skin/qt/stylesheet.cpp
    android/skin/qt/tool-window.cpp
    android/skin/qt/ui-event-recorder.cpp
    android/skin/qt/user-actions-counter.cpp
    android/skin/qt/virtualscene-control-window.cpp
    android/skin/qt/VirtualSceneInfoDialog.cpp
    android/skin/qt/wavefront-obj-parser.cpp
    android/skin/qt/video-player/Clock.cpp
    android/skin/qt/video-player/FrameQueue.cpp
    android/skin/qt/video-player/PacketQueue.cpp
    android/skin/qt/video-player/VideoPlayer.cpp
    android/skin/qt/video-player/VideoPlayerNotifier.cpp
    android/skin/qt/video-player/VideoPlayerWidget.cpp
    android/skin/qt/video-player/VideoInfo.cpp
    android/skin/LibuiAgent.cpp
    android/gpu_frame.cpp
    android/emulator-window.c
    android/emulation/control/ScreenCapturer.cpp
    android/window-agent-impl.cpp
    android/main-common-ui.c
    android/resource.c
    android/recording/audio/AudioProducer.cpp
    android/recording/codecs/audio/VorbisCodec.cpp
    android/recording/codecs/video/VP9Codec.cpp
    android/recording/FfmpegRecorder.cpp
    android/recording/Frame.cpp
    android/recording/GifConverter.cpp
    android/recording/screen-recorder.cpp
    android/recording/video/GuestReadbackWorker.cpp
    android/recording/video/VideoProducer.cpp
    android/recording/video/VideoFrameSharer.cpp
    )

set(ANDROID_SKIN_QT_UI_SRC_FILES
    android/skin/qt/extended.ui
    android/skin/qt/extended-pages/battery-page.ui
    android/skin/qt/extended-pages/bug-report-page.ui
    android/skin/qt/extended-pages/camera-page.ui
    android/skin/qt/extended-pages/camera-virtualscene-subpage.ui
    android/skin/qt/extended-pages/car-data-page.ui
    android/skin/qt/extended-pages/cellular-page.ui
    android/skin/qt/extended-pages/dpad-page.ui
    android/skin/qt/extended-pages/finger-page.ui
    android/skin/qt/extended-pages/google-play-page.ui
    android/skin/qt/extended-pages/help-page.ui
    android/skin/qt/extended-pages/location-page.ui
    android/skin/qt/extended-pages/microphone-page.ui
    android/skin/qt/extended-pages/record-screen-page.ui
    android/skin/qt/extended-pages/rotary-input-page.ui
    android/skin/qt/extended-pages/settings-page.ui
    android/skin/qt/extended-pages/snapshot-page.ui
    android/skin/qt/extended-pages/telephony-page.ui
    android/skin/qt/extended-pages/virtual-sensors-page.ui
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.ui
    android/skin/qt/poster-image-well.ui
    android/skin/qt/tools.ui
    android/skin/qt/virtualscene-controls.ui
    android/skin/qt/virtualscene-info-dialog.ui
)

set(ANDROID_SKIN_QT_MOC_SRC_FILES
    android/skin/qt/accelerometer-3d-widget.h
    android/skin/qt/angle-input-widget.h
    android/skin/qt/editable-slider-widget.h
    android/skin/qt/gl-widget.h
    android/skin/qt/emulator-container.h
    android/skin/qt/emulator-overlay.h
    android/skin/qt/emulator-qt-window.h
    android/skin/qt/emulator-qt-no-window.h
    android/skin/qt/event-capturer.h
    android/skin/qt/event-subscriber.h
    android/skin/qt/extended-pages/battery-page.h
    android/skin/qt/extended-pages/bug-report-page.h
    android/skin/qt/extended-pages/camera-page.h
    android/skin/qt/extended-pages/camera-virtualscene-subpage.h
    android/skin/qt/extended-pages/car-data-page.h
    android/skin/qt/extended-pages/cellular-page.h
    android/skin/qt/extended-pages/dpad-page.h
    android/skin/qt/extended-pages/finger-page.h
    android/skin/qt/extended-pages/google-play-page.h
    android/skin/qt/extended-pages/help-page.h
    android/skin/qt/extended-pages/location-page.h
    android/skin/qt/extended-pages/microphone-page.h
    android/skin/qt/extended-pages/record-screen-page-tasks.h
    android/skin/qt/extended-pages/record-screen-page.h
    android/skin/qt/extended-pages/rotary-input-dial.h
    android/skin/qt/extended-pages/rotary-input-page.h
    android/skin/qt/extended-pages/settings-page.h
    android/skin/qt/extended-pages/snapshot-page.h
    android/skin/qt/extended-pages/telephony-page.h
    android/skin/qt/extended-pages/virtual-sensors-page.h
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h
    android/skin/qt/extended-window.h
    android/skin/qt/raised-material-button.h
    android/skin/qt/ModalOverlay.h
    android/skin/qt/poster-image-well.h
    android/skin/qt/OverlayMessageCenter.h
    android/skin/qt/QtLooperImpl.h
    android/skin/qt/VirtualSceneInfoDialog.h
    android/skin/qt/screen-mask.h
    android/skin/qt/size-tweaker.h
    android/skin/qt/tool-window.h
    android/skin/qt/user-actions-counter.h
    android/skin/qt/virtualscene-control-window.h
    android/skin/qt/video-player/VideoPlayerNotifier.h
    android/skin/qt/video-player/VideoPlayerWidget.h
    android/skin/qt/video-player/VideoInfo.h
    )

set(ANDROID_SKIN_QT_DYNAMIC_RESOURCES
    android/skin/qt/resources.qrc)

set(ANDROID_SKIN_QT_UI_SRC_FILES
    android/skin/qt/static_resources.qrc
    )

if(APPLE)
    set(ANDROID_SKIN_SOURCES ${ANDROID_SKIN_SOURCES} android/skin/qt/mac-native-window.mm)
endif()
if(WIN32 OR ("${LOCAL_OS}" STREQUAL "windows") )
    set(ANDROID_SKIN_SOURCES ${ANDROID_SKIN_SOURCES} android/skin/qt/windows-native-window.cpp)
endif()
if(UNIX AND NOT APPLE)
endif()
# Setup the generator dependencies.
set_source_files_properties(${ANDROID_LIBUI_SRC_FILES} ${PLATFORM_EMUBASE_SOURCES} PROPERTIES OBJECT_DEPENDS ${GENERATED_SOURCES})

add_library(emulator-libui
    ${ANDROID_SKIN_QT_UI_SRC_FILES}
    ${ANDROID_SKIN_QT_DYNAMIC_RESOURCES}
    ${ANDROID_SKIN_QT_MOC_SRC_FILES}
    ${ANDROID_SKIN_QT_UI_SRC_FILES}
    ${ANDROID_SKIN_SOURCES}
    ${ANDROID_LIBUI_SRC_FILES}
)
set_source_files_properties(${ANDROID_LIBUI_SRC_FILES} PROPERTIES OBJECT_DEPENDS ${GENERATED_SOURCES})
target_include_directories(emulator-libui PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(emulator-libui PRIVATE ${QT_TARGET_INCLUDE_DIR})
target_include_directories(emulator-libui PRIVATE ${QT_TARGET_INCLUDE_DIR}/QtCore)
target_include_directories(emulator-libui PRIVATE ${QT_INCLUDE_DIR})
target_include_directories(emulator-libui PRIVATE ${QT_INCLUDE_DIR}/QtCore)
target_include_directories(emulator-libui PRIVATE ${QT_INCLUDE_DIR}/QtGui)
target_include_directories(emulator-libui PRIVATE ${QT_INCLUDE_DIR}/QtSvg)
target_include_directories(emulator-libui PRIVATE ${QT_INCLUDE_DIR}/QtWidgets)


target_compile_definitions(emulator-libui PRIVATE -DUSE_MMX=1)
target_compile_options(emulator-libui PRIVATE -mmmx)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # ffmpeg targets C, so it doesn't care that C++11 requres a space bewteen
    # string literals which are being glued together
    target_compile_options(emulator-libui PRIVATE -Wno-reserved-user-defined-literal)
else()
    target_compile_options(emulator-libui PRIVATE  $<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>)
endif()

