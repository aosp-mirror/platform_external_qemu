# This file defines android-emu library

# Add darwinn external libraries and includes
include(android/darwinn/darwinn.cmake)
android_generate_hw_config()
# This is the set of sources that are common in both the shared libary and the archive. We currently have to split them
# up due to dependencies on external variables/functions that are implemented in other libraries.
set(android-emu-common
    ${ANDROID_HW_CONFIG_H}
    android/adb-server.cpp
    android/avd/generate.cpp
    android/avd/hw-config.c
    android/avd/info.c
    android/avd/scanner.c
    android/avd/util.c
    android/avd/util_wrapper.cpp
    android/avd/BugreportInfo.cpp
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
    android/base/sockets/SocketDrainer.cpp
    android/base/sockets/SocketUtils.cpp
    android/base/sockets/SocketWaiter.cpp
    android/base/threads/internal/ParallelTaskBase.cpp
    android/boot-properties.c
    android/car.cpp
    android/car-cluster.cpp
    android/cmdline-option.cpp
    android/CommonReportedInfo.cpp
    android/console_auth.cpp
    android/cpu_accelerator.cpp
    android/crashreport/CrashSystem.cpp
    android/crashreport/CrashReporter_common.cpp
    android/crashreport/HangDetector.cpp
    android/crashreport/detectors/CrashDetectors.cpp
    android/cros.c
    android/curl-support.c
    android/emuctl-client.cpp
    android/emulation/AdbDebugPipe.cpp
    android/emulation/AdbGuestPipe.cpp
    android/emulation/AdbMessageSniffer.cpp
    android/emulation/AdbHostListener.cpp
    android/emulation/AdbHostServer.cpp
    android/emulation/AndroidAsyncMessagePipe.cpp
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
    android/emulation/control/NopRtcBridge.cpp
    android/emulation/CpuAccelerator.cpp
    android/emulation/CrossSessionSocket.cpp
    android/emulation/DmaMap.cpp
    android/emulation/GoldfishDma.cpp
    android/emulation/GoldfishSyncCommandQueue.cpp
    android/emulation/goldfish_sync.cpp
    android/emulation/hostpipe/HostGoldfishPipe.cpp
    android/emulation/LogcatPipe.cpp
    android/emulation/nand_limits.c
    android/emulation/ParameterList.cpp
    android/emulation/qemud/android_qemud_client.cpp
    android/emulation/qemud/android_qemud_multiplexer.cpp
    android/emulation/qemud/android_qemud_serial.cpp
    android/emulation/qemud/android_qemud_service.cpp
    android/emulation/qemud/android_qemud_sink.cpp
    android/emulation/RefcountPipe.cpp
    android/emulation/serial_line.cpp
    android/emulation/SerialLine.cpp
    android/emulation/SetupParameters.cpp
    android/emulation/testing/TestVmLock.cpp
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
    android/HostHwInfo.cpp
    android/hw-control.c
    android/hw-events.c
    android/hw-fingerprint.c
    android/hw-kmsg.c
    android/hw-lcd.c
    android/hw-qemud.cpp
    android/jpeg-compress.c
    android/kernel/kernel_utils.cpp
    android/loadpng.c
    android/location/Point.cpp
    android/location/Route.cpp
    android/main-help.cpp
    android/main-emugl.cpp
    android/main-kernel-parameters.cpp
    android/metrics/AdbLivenessChecker.cpp
    android/metrics/AsyncMetricsReporter.cpp
    android/metrics/CrashMetricsReporting.cpp
    android/metrics/FileMetricsWriter.cpp
    android/metrics/metrics.cpp
    android/metrics/MetricsPaths.cpp
    android/metrics/MetricsReporter.cpp
    android/metrics/MetricsWriter.cpp
    android/metrics/NullMetricsReporter.cpp
    android/metrics/NullMetricsWriter.cpp
    android/metrics/Percentiles.cpp
    android/metrics/PerfStatReporter.cpp
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
    android/network/NetworkPipe.cpp
    android/network/wifi.cpp
    android/network/WifiForwardClient.cpp
    android/network/WifiForwardPeer.cpp
    android/network/WifiForwardPipe.cpp
    android/network/WifiForwardServer.cpp
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
    android/qemu-tcpdump.c
    android/qt/qt_path.cpp
    android/qt/qt_setup.cpp
    android/resource.c
    android/sdk-controller-socket.c
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
    android/uncompress.cpp
    android/update-check/UpdateChecker.cpp
    android/update-check/VersionExtractor.cpp
    android/user-config.cpp
    android/utils/dns.cpp
    android/utils/Random.cpp
    android/utils/sockets.c
    android/utils/socket_drainer.cpp
    android/utils/looper.cpp
    android/verified-boot/load_config.cpp
    android/wear-agent/android_wear_agent.cpp
    android/wear-agent/WearAgent.cpp
    android/wear-agent/PairUpWearPhone.cpp)

# These are the set of sources for which we know we have dependencies. You can use this as a starting point to figure
# out what can move to a seperate library
set(android_emu_dependent_src
    android/automation/AutomationController.cpp
    android/automation/AutomationEventSink.cpp
    android/camera/camera-common.cpp
    android/camera/camera-format-converters.c
    android/camera/camera-list.cpp
    android/camera/camera-metrics.cpp
    android/camera/camera-service.c
    android/camera/camera-videoplayback.cpp
    android/camera/camera-videoplayback-default-renderer.cpp
    android/camera/camera-videoplayback-render-multiplexer.cpp
    android/camera/camera-videoplayback-video-renderer.cpp
    android/camera/camera-virtualscene.cpp
    android/camera/camera-virtualscene-utils.cpp
    android/emulation/address_space_device.cpp
    android/emulation/address_space_host_memory_allocator.cpp
    android/emulation/control/ScreenCapturer.cpp
    android/emulation/FakeRotatingCameraSensor.cpp
    android/emulation/HostMemoryService.cpp
    android/emulation/Keymaster3.cpp
    android/emulation/QemuMiscPipe.cpp
    android/console.cpp
    android/http_proxy.c
    android/hw-sensors.cpp
    android/main-common.c
    android/main-qemu-parameters.cpp
    android/offworld/OffworldPipe.cpp
    android/physics/AmbientEnvironment.cpp
    android/physics/InertialModel.cpp
    android/physics/PhysicalModel.cpp
    android/qemu-setup.cpp
    android/sensors-port.c
    android/snapshot/SnapshotAPI.cpp
    android/test/checkboot.cpp
    android/videoinjection/VideoInjectionController.cpp
    android/videoplayback/VideoplaybackRenderTarget.cpp
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
    android/virtualscene/WASDInputHandler.cpp)

# The standard archive has all the sources, including those that have external dependencies that we have not properly
# declared yet.
# TODO(jansene): Properly clean up the mutual dependencies and make sure they are not circular
list(APPEND android-emu_src ${android-emu-common} ${android_emu_dependent_src})

# Windows specific sources
set(android-emu_windows_src
    android/opengl/NativeGpuInfo_windows.cpp
    android/snapshot/MemoryWatch_windows.cpp
    android/windows_installer.cpp
    android/camera/camera-capture-windows.cpp
    android/crashreport/CrashReporter_windows.cpp)

# Mac specific sources, these will only be included when building for darwin
set(android-emu_darwin-x86_64_src
    android/camera/camera-capture-mac.m
    android/opengl/NativeGpuInfo_darwin.cpp
    android/snapshot/MemoryWatch_darwin.cpp
    android/opengl/macTouchOpenGL.m
    android/snapshot/MacSegvHandler.cpp
    android/crashreport/CrashReporter_darwin.cpp)

# Linux specific sources.
set(android-emu_linux-x86_64_src
    android/opengl/NativeGpuInfo_linux.cpp
    android/snapshot/MemoryWatch_linux.cpp
    android/camera/camera-capture-linux.c
    android/crashreport/CrashReporter_linux.cpp)


android_add_library(android-emu)

# Note that all these dependencies will propagate to whoever relies on android- emu It will also setup the proper
# include directories, so that android-emu can find all the headers needed for using the library defined below. We
# ideally would like to keep this list small.
target_link_libraries(android-emu
                              PUBLIC
                              emulator-libext4_utils
                              android-emu-base
                              emulator-libsparse
                              emulator-libselinux
                              emulator-libjpeg
                              emulator-libyuv
                              emulator-libwebp
                              emulator-tinyobjloader
                              emulator-libkeymaster3
                              emulator-murmurhash
                              emulator-tinyepoxy
                              emulator-libyuv
                              picosha2
                              # Protobuf dependencies
                              metrics
                              featurecontrol
                              crashreport
                              location
                              emulation
                              snapshot
                              telephony
                              verified-boot
                              automation
                              offworld
                              # Prebuilt libraries
                              breakpad_client
                              curl
                              ssl
                              crypto
                              LibXml2::LibXml2
                              png
                              lz4
                              zlib
)

# Here are the windows library and link dependencies. They are public and will propagate onwards to others that depend
# on android-emu
android_target_link_libraries(android-emu
                              windows
                              PUBLIC
                              emulator-libmman-win32
                              d3d9::d3d9
                              mfuuid::mfuuid
                              # For CoTaskMemFree used in camera-capture-windows.cpp
                              ole32::ole32
                              # For GetPerformanceInfo in CrashService_windows.cpp
                              psapi::psapi
                              # Winsock functions
                              ws2_32::ws2_32
                              # GetNetworkParams() for android/utils/dns.c
                              iphlpapi::iphlpapi)

# These are the libs needed for android-emu on linux.
android_target_link_libraries(android-emu linux-x86_64 PUBLIC darwinn -lrt -lc++)

# Here are the darwin library and link dependencies. They are public and will propagate onwards to others that depend on
# android-emu. You should really only add things that are crucial for this library to link
android_target_link_libraries(android-emu
                              darwin-x86_64
                              PUBLIC
                              "-framework AppKit"
                              "-framework AVFoundation" # For camera-capture-mac.m
                              "-framework Accelerate" # Of course, our camera needs it!
                              "-framework CoreMedia" # Also for the camera.
                              "-framework CoreVideo" # Also for the camera.
                              "-framework IOKit"
                              "-weak_framework Hypervisor"
                              "-framework OpenGL")

target_include_directories(android-emu PUBLIC
                                   # TODO(jansene): The next 2 imply a link dependendency on emugl libs, which
                                   # we have not yet made explicit
                                   ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/host/include
                                   ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/shared
                                   # TODO(jansene): We actually have a hard dependency on qemu-glue
                                   # as there are a lot of externs that are actually defined in qemu2-glue.
                                   # this has to be sorted out,
                                   ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG}
                                   # If you use our library, you get access to our headers.
                                   ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_CURRENT_BINARY_DIR}
                                   ${DARWINN_INCLUDE_DIRS})

android_target_compile_options(android-emu Clang PRIVATE -Wno-extern-c-compat -Wno-invalid-constexpr -fvisibility=default)
android_target_compile_options(android-emu linux-x86_64 PRIVATE -idirafter ${ANDROID_QEMU2_TOP_DIR}/linux-headers)
android_target_compile_options(android-emu darwin-x86_64 PRIVATE -Wno-error -Wno-objc-method-access -Wno-missing-selector-name -Wno-receiver-expr -Wno-incomplete-implementation -Wno-incompatible-pointer-types)

android_target_compile_definitions(android-emu
                                   darwin-x86_64
                                   PRIVATE
                                   "-D_DARWIN_C_SOURCE=1"
                                   "-Dftello64=ftell"
                                   "-Dfseeko64=fseek")

target_compile_definitions(android-emu
                                   PRIVATE
                                   "-DCRASHUPLOAD=${OPTION_CRASHUPLOAD}"
                                   "-DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}"
                                   "-DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}")

if(WEBRTC)
    target_compile_definitions(android-emu PUBLIC -DANDROID_WEBRTC)
endif()

if (GRPC)
    target_compile_definitions(android-emu PUBLIC -DANDROID_GRPC)
endif()

# Boo, we need the make_ext4fs executable
add_dependencies(android-emu emulator_make_ext4fs)

# Shared version of the library. Note that this only has the set of common sources, otherwise you will get a lot of
# linker errors.
set(android-emu-shared_src ${android-emu-common} stubs/stubs.cpp)

# The dependent target os specific sources, they are pretty much the same as above, excluding camera support, because
# that brings in a whole slew of dependencies.

# Windows specific sources
set(android-emu-shared_windows_src android/opengl/NativeGpuInfo_windows.cpp android/snapshot/MemoryWatch_windows.cpp
    android/windows_installer.cpp android/crashreport/CrashReporter_windows.cpp)

# Mac specific sources, these will only be included when building for darwin
set(android-emu-shared_darwin-x86_64_src
    android/opengl/NativeGpuInfo_darwin.cpp
    android/snapshot/MemoryWatch_darwin.cpp
    android/opengl/macTouchOpenGL.m
    android/snapshot/MacSegvHandler.cpp
    android/crashreport/CrashReporter_darwin.cpp)

# Linux specific sources.
set(android-emu-shared_linux-x86_64_src android/opengl/NativeGpuInfo_linux.cpp android/snapshot/MemoryWatch_linux.cpp
    android/crashreport/CrashReporter_linux.cpp)

android_add_shared_library(android-emu-shared)

# Note that these are basically the same as android-emu-shared. We should clean this up
target_link_libraries(android-emu-shared
                              PRIVATE
                              emulator-libext4_utils
                              android-emu-base
                              emulator-libsparse
                              emulator-libselinux
                              emulator-libjpeg
                              emulator-libyuv
                              emulator-libwebp
                              emulator-tinyobjloader
                              emulator-libkeymaster3
                              emulator-murmurhash
                              emulator-tinyepoxy
                              emulator-libyuv
                              picosha2
                              # Protobuf dependencies
                              metrics
                              featurecontrol
                              crashreport
                              location
                              emulation
                              snapshot
                              telephony
                              verified-boot
                              automation
                              offworld
                              # Prebuilt libraries
                              breakpad_client
                              curl
                              crypto
                              crypto
                              LibXml2::LibXml2
                              png
                              lz4
                              zlib)

# Here are the windows library and link dependencies. They are public and will propagate onwards to others that depend
# on android-emu-shared
android_target_link_libraries(android-emu-shared
                              windows
                              PRIVATE
                              emulator-libmman-win32
                              d3d9::d3d9
                              # IID_IMFSourceReaderCallback
                              mfuuid::mfuuid
                              # For CoTaskMemFree used in camera-capture-windows.cpp
                              ole32::ole32
                              # For GetPerformanceInfo in CrashService_windows.cpp
                              psapi::psapi
                              # Winsock functions
                              ws2_32::ws2_32
                              # GetNetworkParams() for android/utils/dns.c
                              iphlpapi::iphlpapi
)


# These are the libs needed for android-emu-shared on linux.
android_target_link_libraries(android-emu-shared linux-x86_64 PRIVATE -lrt)

# Here are the darwin library and link dependencies. They are public and will propagate onwards to others that depend on
# android-emu-shared. You should really only add things that are crucial for this library to link
# If you don't you might see bizarre errors. (Add opengl as a link dependency, you will have fun)
android_target_link_libraries(android-emu-shared
                              darwin-x86_64
                              PRIVATE
                              "-framework AppKit"
)

target_include_directories(android-emu-shared PUBLIC
                                   # TODO(jansene): The next 2 imply a link dependendency on emugl libs, which
                                   # we have not yet made explicit
                                   ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/host/include
                                   ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/shared
                                   # TODO(jansene): We actually have a hard dependency on qemu-glue
                                   # as there are a lot of externs that are actually defined in qemu2-glue.
                                   # this has to be sorted out,
                                   ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG}
                                   # If you use our library, you get access to our headers.
                                   ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

android_target_compile_options(android-emu-shared Clang PRIVATE -Wno-extern-c-compat -Wno-invalid-constexpr -fvisibility=default)
android_target_compile_options(android-emu-shared linux-x86_64 PRIVATE -idirafter ${ANDROID_QEMU2_TOP_DIR}/linux-headers)
android_target_compile_options(android-emu-shared darwin-x86_64 PRIVATE -Wno-error -Wno-objc-method-access -Wno-receiver-expr -Wno-incomplete-implementation -Wno-missing-selector-name -Wno-incompatible-pointer-types)

# Definitions needed to compile our deps as static
target_compile_definitions(android-emu-shared PUBLIC ${CURL_DEFINITIONS} ${LIBXML2_DEFINITIONS})
android_target_compile_definitions(android-emu-shared
                                   darwin-x86_64
                                   PRIVATE
                                   "-D_DARWIN_C_SOURCE=1"
                                   "-Dftello64=ftell"
                                   "-Dfseeko64=fseek")

target_compile_definitions(android-emu-shared
                                   PRIVATE
                                   "-DCRASHUPLOAD=${OPTION_CRASHUPLOAD}"
                                   "-DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}"
                                   "-DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}")

if(WEBRTC)
    target_compile_definitions(android-emu-shared PUBLIC -DANDROID_WEBRTC)
endif()

if (GRPC)
    target_compile_definitions(android-emu-shared PUBLIC -DANDROID_GRPC)
endif()

set(android-mock-vm-operations_src
    android/emulation/testing/MockAndroidVmOperations.cpp)

android_add_library(android-mock-vm-operations)

android_target_compile_options(android-mock-vm-operations Clang PRIVATE -O0 -Wno-invalid-constexpr)
target_include_directories(android-mock-vm-operations PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(android-mock-vm-operations PRIVATE gmock)

# The unit tests
set(android-emu_unittests_src
    android/automation/AutomationController_unittest.cpp
    android/automation/AutomationEventSink_unittest.cpp
    android/avd/util_unittest.cpp
    android/avd/util_wrapper_unittest.cpp
    android/base/ArraySize_unittest.cpp
    android/base/AlignedBuf_unittest.cpp
    android/base/ContiguousRangeMapper_unittest.cpp
    android/base/async/Looper_unittest.cpp
    android/base/async/AsyncSocketServer_unittest.cpp
    android/base/async/RecurrentTask_unittest.cpp
    android/base/async/ScopedSocketWatch_unittest.cpp
    android/base/async/SubscriberList_unittest.cpp
    android/base/containers/BufferQueue_unittest.cpp
    android/base/containers/CircularBuffer_unittest.cpp
    android/base/containers/EntityManager_unittest.cpp
    android/base/containers/Lookup_unittest.cpp
    android/base/containers/SmallVector_unittest.cpp
    android/base/containers/StaticMap_unittest.cpp
    android/base/EintrWrapper_unittest.cpp
    android/base/files/FileShareOpen_unittest.cpp
    android/base/files/IniFile_unittest.cpp
    android/base/files/InplaceStream_unittest.cpp
    android/base/files/MemStream_unittest.cpp
    android/base/files/PathUtils_unittest.cpp
    android/base/files/ScopedFd_unittest.cpp
    android/base/files/ScopedStdioFile_unittest.cpp
    android/base/files/Stream_unittest.cpp
    android/base/files/StreamSerializing_unittest.cpp
    android/base/FunctionView_unittest.cpp
    android/base/JsonWriter_unittest.cpp
    android/base/Log_unittest.cpp
    android/base/memory/LazyInstance_unittest.cpp
    android/base/memory/MemoryHints_unittest.cpp
    android/base/memory/MallocUsableSize_unittest.cpp
    android/base/memory/OnDemand_unittest.cpp
    android/base/memory/ScopedPtr_unittest.cpp
    android/base/memory/SharedMemory_unittest.cpp
    android/base/misc/FileUtils_unittest.cpp
    android/base/misc/HttpUtils_unittest.cpp
    android/base/misc/IpcPipe_unittest.cpp
    android/base/misc/StringUtils_unittest.cpp
    android/base/misc/Utf8Utils_unittest.cpp
    android/base/network/Dns_unittest.cpp
    android/base/network/IpAddress_unittest.cpp
    android/base/network/NetworkUtils_unittest.cpp
    android/base/Optional_unittest.cpp
    android/base/perflogger/Benchmark_unittest.cpp
    android/base/Pool_unittest.cpp
    android/base/ProcessControl_unittest.cpp
    android/base/Result_unittest.cpp
    android/base/sockets/ScopedSocket_unittest.cpp
    android/base/sockets/SocketDrainer_unittest.cpp
    android/base/sockets/SocketUtils_unittest.cpp
    android/base/sockets/SocketWaiter_unittest.cpp
    android/base/StringFormat_unittest.cpp
    android/base/StringParse_unittest.cpp
    android/base/StringView_unittest.cpp
    android/base/SubAllocator_unittest.cpp
    android/base/synchronization/ConditionVariable_unittest.cpp
    android/base/synchronization/Lock_unittest.cpp
    android/base/synchronization/ReadWriteLock_unittest.cpp
    android/base/synchronization/MessageChannel_unittest.cpp
    android/base/system/System_unittest.cpp
    android/base/testing/MockUtils_unittest.cpp
    android/base/testing/ProtobufMatchers.cpp
    android/base/testing/TestEvent_unittest.cpp
    android/base/threads/Async_unittest.cpp
    android/base/threads/FunctorThread_unittest.cpp
    android/base/threads/ParallelTask_unittest.cpp
    android/base/threads/Thread_unittest.cpp
    android/base/threads/ThreadStore_unittest.cpp
    android/base/TypeTraits_unittest.cpp
    android/base/Uri_unittest.cpp
    android/base/Uuid_unittest.cpp
    android/base/Version_unittest.cpp
    android/camera/CameraFormatConverters_unittest.cpp
    android/cmdline-option_unittest.cpp
    android/CommonReportedInfo_unittest.cpp
    android/console_auth_unittest.cpp
    android/console_unittest.cpp
    android/emulation/AdbDebugPipe_unittest.cpp
    android/emulation/AdbGuestPipe_unittest.cpp
    android/emulation/AdbHostListener_unittest.cpp
    android/emulation/AdbHostServer_unittest.cpp
    android/emulation/android_pipe_pingpong_unittest.cpp
    android/emulation/android_pipe_zero_unittest.cpp
    android/emulation/AndroidAsyncMessagePipe_unittest.cpp
    android/emulation/bufprint_config_dirs_unittest.cpp
    android/emulation/ComponentVersion_unittest.cpp
    android/emulation/ConfigDirs_unittest.cpp
    android/emulation/DeviceContextRunner_unittest.cpp
    android/emulation/DmaMap_unittest.cpp
    android/emulation/control/AdbInterface_unittest.cpp
    android/emulation/control/ApkInstaller_unittest.cpp
    android/emulation/control/FilePusher_unittest.cpp
    android/emulation/control/GooglePlayServices_unittest.cpp
    android/emulation/control/ScreenCapturer_unittest.cpp
    android/emulation/control/LineConsumer_unittest.cpp
    android/emulation/CpuAccelerator_unittest.cpp
    android/emulation/CrossSessionSocket_unittest.cpp
    android/emulation/HostMemoryService_unittest.cpp
    android/emulation/Hypervisor_unittest.cpp
    android/emulation/hostpipe/HostGoldfishPipe_unittest.cpp
    android/emulation/ParameterList_unittest.cpp
    android/emulation/RefcountPipe_unittest.cpp
    android/emulation/serial_line_unittest.cpp
    android/emulation/SetupParameters_unittest.cpp
    android/emulation/testing/TestAndroidPipeDevice.cpp
    android/emulation/testing/MockAndroidEmulatorWindowAgent.cpp
    android/emulation/VmLock_unittest.cpp
    android/error-messages_unittest.cpp
    android/featurecontrol/FeatureControl_unittest.cpp
    android/featurecontrol/HWMatching_unittest.cpp
    android/featurecontrol/testing/FeatureControlTest.cpp
    android/filesystems/ext4_resize_unittest.cpp
    android/filesystems/ext4_utils_unittest.cpp
    android/filesystems/fstab_parser_unittest.cpp
    android/filesystems/partition_config_unittest.cpp
    android/filesystems/partition_types_unittest.cpp
    android/filesystems/ramdisk_extractor_unittest.cpp
    android/filesystems/testing/TestSupport.cpp
    android/gps/GpxParser_unittest.cpp
    android/gps/KmlParser_unittest.cpp
    android/hw-lcd_unittest.cpp
    android/kernel/kernel_utils_unittest.cpp
    android/location/Point_unittest.cpp
    android/location/Route_unittest.cpp
    android/network/control_unittest.cpp
    android/network/constants_unittest.cpp
    android/offworld/OffworldPipe_unittest.cpp
    android/opengl/EmuglBackendList_unittest.cpp
    android/opengl/EmuglBackendScanner_unittest.cpp
    android/opengl/emugl_config_unittest.cpp
    android/opengl/GpuFrameBridge_unittest.cpp
    android/opengl/gpuinfo_unittest.cpp
    android/physics/AmbientEnvironment_unittest.cpp
    android/physics/InertialModel_unittest.cpp
    android/physics/PhysicalModel_unittest.cpp
    android/proxy/proxy_common_unittest.cpp
    android/proxy/ProxyUtils_unittest.cpp
    android/qt/qt_path_unittest.cpp
    android/qt/qt_setup_unittest.cpp
    android/snapshot/RamLoader_unittest.cpp
    android/snapshot/RamSaver_unittest.cpp
    android/snapshot/RamSnapshot_unittest.cpp
    android/snapshot/Snapshot_unittest.cpp
    android/telephony/gsm_unittest.cpp
    android/telephony/modem_unittest.cpp
    android/telephony/sms_unittest.cpp
    android/telephony/SimAccessRules_unittest.cpp
    android/telephony/TagLengthValue_unittest.cpp
    android/update-check/UpdateChecker_unittest.cpp
    android/update-check/VersionExtractor_unittest.cpp
    android/utils/aconfig-file_unittest.cpp
    android/utils/bufprint_unittest.cpp
    android/utils/dirscanner_unittest.cpp
    android/utils/dns_unittest.cpp
    android/utils/eintr_wrapper_unittest.cpp
    android/utils/file_data_unittest.cpp
    android/utils/filelock_unittest.cpp
    android/utils/format_unittest.cpp
    android/utils/host_bitness_unittest.cpp
    android/utils/path_unittest.cpp
    android/utils/property_file_unittest.cpp
    android/utils/Random_unittest.cpp
    android/utils/string_unittest.cpp
    android/utils/sockets_unittest.cpp
    android/utils/x86_cpuid_unittest.cpp
    android/verified-boot/load_config_unittest.cpp
    android/videoinjection/VideoInjectionController_unittest.cpp
    android/virtualscene/TextureUtils_unittest.cpp
    android/wear-agent/PairUpWearPhone_unittest.cpp
    android/wear-agent/testing/WearAgentTestUtils.cpp
    android/wear-agent/WearAgent_unittest.cpp)

# Windows specific unit tests
set(android-emu_unittests_windows_src
    android/base/files/ScopedFileHandle_unittest.cpp
    android/base/files/ScopedRegKey_unittest.cpp
    android/base/system/Win32UnicodeString_unittest.cpp
    android/base/system/Win32Utils_unittest.cpp
    android/utils/win32_cmdline_quote_unittest.cpp
    android/windows_installer_unittest.cpp)

# msvc specific unittests
set(android-emu_unittests_windows_msvc-x86_64_src
    android/base/system/WinMsvcSystem_unittest.cpp)

# Darwin & Linux only tests
set(android-emu_unittests_darwin-x86_64_src android/emulation/nand_limits_unittest.cpp)

set(android-emu_unittests_linux-x86_64_src
    android/emulation/nand_limits_unittest.cpp)

# And declare the test
android_add_test(android-emu_unittests)

# Setup the targets compile config etc..
android_target_compile_options(android-emu_unittests Clang PRIVATE -O0 -Wno-invalid-constexpr)
target_include_directories(android-emu_unittests PRIVATE ../android-emugl/host/include/)

target_compile_definitions(android-emu_unittests PRIVATE -DGTEST_HAS_RTTI=0)

# Settings needed for darwin
android_target_compile_definitions(android-emu_unittests
                                   darwin-x86_64
                                   PRIVATE
                                   "-D_DARWIN_C_SOURCE=1"
                                   "-Dftello64=ftell"
                                   "-Dfseeko64=fseek")

# Dependecies are exported from android-emu.
target_link_libraries(android-emu_unittests PRIVATE android-emu android-mock-vm-operations gtest gmock gtest_main)

list(APPEND android-emu-testdata
            testdata/snapshots/random-ram-100.bin
            testdata/textureutils/gray_alpha_golden.bmp
            testdata/textureutils/gray_alpha.png
            testdata/textureutils/gray_golden.bmp
            testdata/textureutils/gray.png
            testdata/textureutils/indexed_alpha_golden.bmp
            testdata/textureutils/indexed_alpha.png
            testdata/textureutils/indexed_golden.bmp
            testdata/textureutils/indexed.png
            testdata/textureutils/interlaced_golden.bmp
            testdata/textureutils/interlaced.png
            testdata/textureutils/jpeg_gray_golden.bmp
            testdata/textureutils/jpeg_gray.jpg
            testdata/textureutils/jpeg_gray_progressive_golden.bmp
            testdata/textureutils/jpeg_gray_progressive.jpg
            testdata/textureutils/jpeg_rgb24_golden.bmp
            testdata/textureutils/jpeg_rgb24.jpg
            testdata/textureutils/jpeg_rgb24_progressive_golden.bmp
            testdata/textureutils/jpeg_rgb24_progressive.jpg
            testdata/textureutils/rgb24_31px_golden.bmp
            testdata/textureutils/rgb24_31px.png
            testdata/textureutils/rgba32_golden.bmp
            testdata/textureutils/rgba32.png)

prebuilt(VIRTUALSCENE)
android_copy_test_files(android-emu_unittests "${android-emu-testdata}" testdata)
android_target_dependency(android-emu_unittests all VIRTUAL_SCENE_DEPENDENCIES)
android_copy_test_dir(android-emu_unittests test-sdk test-sdk)
android_copy_file(android-emu_unittests  "${CMAKE_CURRENT_SOURCE_DIR}/android/emulation/CpuAccelerator_unittest.dat" "$<TARGET_FILE_DIR:android-emu_unittests>/android/android-emu/android/emulation/CpuAccelerator_unittest.dat")
android_copy_file(android-emu_unittests  "${CMAKE_CURRENT_SOURCE_DIR}/android/emulation/CpuAccelerator_unittest.dat2" "$<TARGET_FILE_DIR:android-emu_unittests>/android/android-emu/android/emulation/CpuAccelerator_unittest.dat2")

android_target_dependency(android-emu_unittests all E2FSPROGS_DEPENDENCIES)

# Boo! We depend on makeext
add_custom_command(TARGET android-emu_unittests POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:emulator_make_ext4fs> ${CMAKE_CURRENT_BINARY_DIR})

# Unit tests for the protobufs
set(android-emu-metrics_unittests_src
    android/metrics/StudioConfig_unittest.cpp
    android/metrics/tests/AsyncMetricsReporter_unittest.cpp
    android/metrics/tests/FileMetricsWriter_unittest.cpp
    android/metrics/tests/MetricsReporter_unittest.cpp
    android/metrics/tests/MockMetricsReporter.cpp
    android/metrics/tests/MockMetricsWriter.cpp
    android/metrics/tests/NullMetricsClasses_unittest.cpp
    android/metrics/tests/Percentiles_unittest.cpp
    android/metrics/tests/PeriodicReporter_unittest.cpp
    android/metrics/tests/SyncMetricsReporter_unittest.cpp)
android_add_test(android-emu-metrics_unittests)

target_compile_options(android-emu-metrics_unittests PRIVATE -O0)
target_link_libraries(android-emu-metrics_unittests PRIVATE gmock_main android-emu)
