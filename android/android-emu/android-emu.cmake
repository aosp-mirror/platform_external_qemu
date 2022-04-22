# This file defines android-emu library
prebuilt(VPX)
prebuilt(FFMPEG)

# This is a minimal emu-android library needed to start the emulator. - It has a
# minimal number of dependencies - It should *NEVER* rely on a shared library
# (binplace issues MSVC) - It is used by the "emulator" launcher binary to
# provide basic functionality (launch, find avds, list camera's etc.)
#
# This allows us to create shared libraries without negatively impacting the
# launcher.
android_add_library(
  TARGET android-emu-launch
  LICENSE Apache-2.0
  SRC android/avd/hw-config.c
      android/avd/info.c
      android/avd/scanner.c
      android/avd/util.c
      android/avd/util_wrapper.cpp
      android/camera/camera-common.cpp
      android/camera/camera-format-converters.c
      android/camera/camera-list.cpp
      android/emulation/bufprint_config_dirs.cpp
      android/emulation/CpuAccelerator.cpp
      android/emulation/launcher/launcher_stub.cpp
      android/help.c
      android/HostHwInfo.cpp
      android/hw-lcd.c
      android/main-help.cpp
      android/network/constants.c
      android/utils/exec.cpp
      android/utils/host_bitness.cpp
      android/utils/system_wrapper.cpp
  WINDOWS android/camera/camera-capture-windows.cpp
          android/emulation/USBAssist.cpp
  LINUX android/camera/camera-capture-linux.c
  DARWIN android/camera/camera-capture-mac.m)
target_include_directories(android-emu-launch PRIVATE .)
target_link_libraries(
  android-emu-launch PRIVATE android-emu-base android-files emulator-libyuv
                             android-hw-config gtest)
target_compile_options(android-emu-launch PRIVATE -Wno-extern-c-compat)
target_compile_definitions(android-emu-launch PRIVATE AEMU_MIN AEMU_LAUNCHER)
target_include_directories(
  android-emu-launch
  PRIVATE
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG})

android_target_link_libraries(
  android-emu-launch darwin
  PUBLIC "-framework AVFoundation" "-framework CoreMedia"
         "-framework CoreVideo" "-framework Accelerate")

android_target_link_libraries(
  android-emu-launch windows PRIVATE emulator-libusb ole32::ole32
                                     setupapi::setupapi mfuuid::mfuuid)

# This is the set of sources that are common in both the shared libary and the
# archive. We currently have to split them up due to dependencies on external
# variables/functions that are implemented in other libraries.
set(android-emu-common
    # cmake-format: sortable
    android/adb-server.cpp
    android/async-console.c
    android/async-socket-connector.c
    android/async-socket.c
    android/async-utils.c
    android/avd/BugreportInfo.cpp
    android/avd/generate.cpp
    android/avd/hw-config.c
    android/avd/info.c
    android/avd/scanner.c
    android/avd/util.c
    android/avd/util_wrapper.cpp
    android/base/async/AsyncSocket.cpp
    android/base/async/CallbackRegistry.cpp
    android/base/LayoutResolver.cpp
    android/boot-properties.c
    android/bootconfig.cpp
    android/car-cluster.cpp
    android/car.cpp
    android/cmdline-option.cpp
    android/CommonReportedInfo.cpp
    android/console_auth.cpp
    android/cpu_accelerator.cpp
    android/crashreport/CrashReporter_common.cpp
    android/crashreport/CrashSystem.cpp
    android/crashreport/detectors/CrashDetectors.cpp
    android/crashreport/HangDetector.cpp
    android/cros.c
    android/emulation/AdbDebugPipe.cpp
    android/emulation/AdbGuestPipe.cpp
    android/emulation/AdbHostListener.cpp
    android/emulation/AdbHostServer.cpp
    android/emulation/AdbHub.cpp
    android/emulation/AdbMessageSniffer.cpp
    android/emulation/AdbVsockPipe.cpp
    android/emulation/address_space_device.cpp
    android/emulation/address_space_graphics.cpp
    android/emulation/address_space_host_media.cpp
    android/emulation/address_space_host_memory_allocator.cpp
    android/emulation/address_space_shared_slots_host_memory_allocator.cpp
    android/emulation/android_pipe_host.cpp
    android/emulation/android_pipe_pingpong.c
    android/emulation/android_pipe_throttle.c
    android/emulation/android_pipe_unix.cpp
    android/emulation/android_pipe_zero.c
    android/emulation/android_qemud.cpp
    android/emulation/AndroidAsyncMessagePipe.cpp
    android/emulation/AndroidMessagePipe.cpp
    android/emulation/AndroidPipe.cpp
    android/emulation/AudioCaptureEngine.cpp
    android/emulation/AudioOutputEngine.cpp
    android/emulation/bufprint_config_dirs.cpp
    android/emulation/ClipboardPipe.cpp
    android/emulation/ComponentVersion.cpp
    android/emulation/control/adb/AdbConnection.cpp
    android/emulation/control/adb/AdbInterface.cpp
    android/emulation/control/adb/adbkey.cpp
    android/emulation/control/adb/AdbShellStream.cpp
    android/emulation/control/AgentLogger.cpp
    android/emulation/control/ApkInstaller.cpp
    android/emulation/control/EmulatorAdvertisement.cpp
    android/emulation/control/FilePusher.cpp
    android/emulation/control/GooglePlayServices.cpp
    android/emulation/control/LineConsumer.cpp
    android/emulation/control/NopRtcBridge.cpp
    android/emulation/control/ServiceUtils.cpp
    android/emulation/CpuAccelerator.cpp
    android/emulation/CrossSessionSocket.cpp
    android/emulation/DmaMap.cpp
    android/emulation/goldfish_sync.cpp
    android/emulation/GoldfishDma.cpp
    android/emulation/GoldfishSyncCommandQueue.cpp
    android/emulation/H264NaluParser.cpp
    android/emulation/H264PingInfoParser.cpp
    android/emulation/HostapdController.cpp
    android/emulation/hostdevices/HostAddressSpace.cpp
    android/emulation/hostdevices/HostGoldfishPipe.cpp
    android/emulation/HostmemIdMapping.cpp
    android/emulation/LogcatPipe.cpp
    android/emulation/MediaFfmpegVideoHelper.cpp
    android/emulation/MediaH264Decoder.cpp
    android/emulation/MediaH264DecoderDefault.cpp
    android/emulation/MediaH264DecoderGeneric.cpp
    android/emulation/MediaHostRenderer.cpp
    android/emulation/MediaSnapshotHelper.cpp
    android/emulation/MediaSnapshotState.cpp
    android/emulation/MediaTexturePool.cpp
    android/emulation/MediaVideoHelper.cpp
    android/emulation/MediaVpxDecoder.cpp
    android/emulation/MediaVpxDecoderGeneric.cpp
    android/emulation/MediaVpxVideoHelper.cpp
    android/emulation/MultiDisplay.cpp
    android/emulation/MultiDisplayPipe.cpp
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
    android/emulation/USBAssist.cpp
    android/emulation/VmLock.cpp
    android/emulation/VpxFrameParser.cpp
    android/emulation/VpxPingInfoParser.cpp
    android/error-messages.cpp
    android/featurecontrol/feature_control.cpp
    android/featurecontrol/FeatureControl.cpp
    android/featurecontrol/FeatureControlImpl.cpp
    android/featurecontrol/HWMatching.cpp
    android/framebuffer.c
    android/gps.c
    android/gps/GpxParser.cpp
    android/gps/KmlParser.cpp
    android/gps/PassiveGpsUpdater.cpp
    android/gpu_frame.cpp
    android/help.c
    android/HostHwInfo.cpp
    android/hw-control.cpp
    android/hw-events.c
    android/hw-fingerprint.c
    android/hw-kmsg.c
    android/hw-lcd.c
    android/hw-qemud.cpp
    android/jdwp/JdwpProxy.cpp
    android/jpeg-compress.c
    android/kernel/kernel_utils.cpp
    android/loadpng.c
    android/location/MapsKey.cpp
    android/location/MapsKeyFileParser.cpp
    android/location/Point.cpp
    android/location/Route.cpp
    android/location/StudioMapsKey.cpp
    android/main-emugl.cpp
    android/main-help.cpp
    android/main-kernel-parameters.cpp
    android/metrics/AdbLivenessChecker.cpp
    android/metrics/DependentMetrics.cpp
    android/metrics/PerfStatReporter.cpp
    android/multi-instance.cpp
    android/multitouch-port.c
    android/multitouch-screen.c
    android/network/constants.c
    android/network/control.cpp
    android/network/GenericNetlinkMessage.cpp
    android/network/globals.c
    android/network/Ieee80211Frame.cpp
    android/network/NetworkPipe.cpp
    android/network/wifi.cpp
    android/network/WifiForwardClient.cpp
    android/network/WifiForwardPeer.cpp
    android/network/WifiForwardPipe.cpp
    android/network/WifiForwardServer.cpp
    android/opengl/emugl_config.cpp
    android/opengl/EmuglBackendList.cpp
    android/opengl/EmuglBackendScanner.cpp
    android/opengl/GLProcessPipe.cpp
    android/opengl/GpuFrameBridge.cpp
    android/opengl/gpuinfo.cpp
    android/opengl/logger.cpp
    android/opengl/OpenglEsPipe.cpp
    android/opengles.cpp
    android/process_setup.cpp
    android/protobuf/LoadSave.cpp
    android/protobuf/ProtobufLogging.cpp
    android/proxy/proxy_common.c
    android/proxy/proxy_http.c
    android/proxy/proxy_http_connector.c
    android/proxy/proxy_http_rewriter.c
    android/proxy/proxy_setup.cpp
    android/proxy/ProxyUtils.cpp
    android/qemu-tcpdump.c
    android/resource.c
    android/sdk-controller-socket.c
    android/sensor_mock/SensorMockUtils.cpp
    android/sensor_replay/sensor_session_playback.cpp
    android/session_phase_reporter.cpp
    android/shaper.c
    android/skin/charmap.c
    android/snaphost-android.c
    android/snapshot.c
    android/snapshot/common.cpp
    android/snapshot/Compressor.cpp
    android/snapshot/Decompressor.cpp
    android/snapshot/GapTracker.cpp
    android/snapshot/Hierarchy.cpp
    android/snapshot/IncrementalStats.cpp
    android/snapshot/interface.cpp
    android/snapshot/Loader.cpp
    android/snapshot/MemoryWatch_common.cpp
    android/snapshot/PathUtils.cpp
    android/snapshot/Quickboot.cpp
    android/snapshot/RamLoader.cpp
    android/snapshot/RamSaver.cpp
    android/snapshot/RamSnapshotTesting.cpp
    android/snapshot/Saver.cpp
    android/snapshot/Snapshot.cpp
    android/snapshot/Snapshotter.cpp
    android/snapshot/SnapshotUtils.cpp
    android/snapshot/TextureLoader.cpp
    android/snapshot/TextureSaver.cpp
    android/telephony/debug.c
    android/telephony/gsm.c
    android/telephony/MeterService.cpp
    android/telephony/modem.c
    android/telephony/modem_driver.c
    android/telephony/phone_number.cpp
    android/telephony/remote_call.c
    android/telephony/sim_card.c
    android/telephony/SimAccessRules.cpp
    android/telephony/sms.c
    android/telephony/sysdeps.c
    android/telephony/TagLengthValue.cpp
    android/update-check/UpdateChecker.cpp
    android/update-check/VersionExtractor.cpp
    android/user-config.cpp
    android/userspace-boot-properties.cpp
    android/utils/assert.c
    android/utils/async.cpp
    android/utils/cbuffer.c
    android/utils/dns.cpp
    android/utils/exec.cpp
    android/utils/format.cpp
    android/utils/host_bitness.cpp
    android/utils/http_utils.cpp
    android/utils/intmap.c
    android/utils/iolooper.cpp
    android/utils/ipaddr.cpp
    android/utils/lineinput.c
    android/utils/Random.cpp
    android/utils/reflist.c
    android/utils/refset.c
    android/utils/system_wrapper.cpp
    android/utils/timezone.cpp
    android/utils/uri.cpp
    android/utils/utf8_utils.cpp
    android/utils/vector.c
    android/verified-boot/load_config.cpp
    android/wear-agent/android_wear_agent.cpp
    android/wear-agent/PairUpWearPhone.cpp
    android/wear-agent/WearAgent.cpp)

# These are the set of sources for which we know we have dependencies. You can
# use this as a starting point to figure out what can move to a seperate library
set(android_emu_dependent_src
    # cmake-format: sortable
    android/automation/AutomationController.cpp
    android/automation/AutomationEventSink.cpp
    android/camera/camera-common.cpp
    android/camera/camera-format-converters.c
    android/camera/camera-list.cpp
    android/camera/camera-metrics.cpp
    android/camera/camera-service.cpp
    android/camera/camera-videoplayback-default-renderer.cpp
    android/camera/camera-videoplayback-render-multiplexer.cpp
    android/camera/camera-videoplayback-video-renderer.cpp
    android/camera/camera-videoplayback.cpp
    android/camera/camera-virtualscene-utils.cpp
    android/camera/camera-virtualscene.cpp
    android/console.cpp
    android/emulation/control/ScreenCapturer.cpp
    android/emulation/FakeRotatingCameraSensor.cpp
    android/emulation/HostMemoryService.cpp
    android/emulation/QemuMiscPipe.cpp
    android/emulation/resizable_display_config.cpp
    android/http_proxy.c
    android/hw-sensors.cpp
    android/main-common.c
    android/main-qemu-parameters.cpp
    android/offworld/OffworldPipe.cpp
    android/physics/AmbientEnvironment.cpp
    android/physics/BodyModel.cpp
    android/physics/FoldableModel.cpp
    android/physics/InertialModel.cpp
    android/physics/PhysicalModel.cpp
    android/qemu-setup.cpp
    android/sensors-port.c
    android/snapshot/Icebox.cpp
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

android_add_library(TARGET android-emu-agents SHARED LICENSE Apache-2.0
                    SRC android/emulation/control/AndroidAgentFactory.cpp)

android_target_properties(android-emu-agents darwin
                          "INSTALL_RPATH>=@loader_path/gles_swiftshader")
target_include_directories(
  android-emu-agents
  PUBLIC .
  PRIVATE
    # TODO(jansene): We actually have a hard dependency on qemu-glue as there
    # are a lot of externs that are actually defined in qemu2-glue. this has to
    # be sorted out,
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG})
target_link_libraries(android-emu-agents PRIVATE android-emu-base android-files
                                                 android-hw-config)
target_compile_options(android-emu-agents PRIVATE "-Wno-extern-c-compat")
target_compile_definitions(android-emu-agents PRIVATE "CONSOLE_EXPORTS")
android_install_shared(android-emu-agents)
if(WINDOWS_MSVC_X86_64)
  install(TARGETS android-emu-agents DESTINATION .)
endif()

# The standard archive has all the sources, including those that have external
# dependencies that we have not properly declared yet.
# TODO(jansene): Properly clean up the mutual dependencies and make sure they
# are not circular
list(APPEND android-emu_src ${android-emu-common} ${android_emu_dependent_src})

android_add_library(
  TARGET android-emu
  LICENSE Apache-2.0
  SRC ${android-emu_src}
  WINDOWS # cmake-format: sortable
          android/camera/camera-capture-windows.cpp
          android/crashreport/CrashReporter_windows.cpp
          android/emulation/dynlink_cuda.cpp
          android/emulation/dynlink_nvcuvid.cpp
          android/emulation/MediaCudaDriverHelper.cpp
          android/emulation/MediaCudaUtils.cpp
          android/emulation/MediaCudaVideoHelper.cpp
          android/opengl/NativeGpuInfo_windows.cpp
          android/snapshot/MemoryWatch_windows.cpp
          android/windows_installer.cpp
  LINUX # cmake-format: sortable
        android/camera/camera-capture-linux.c
        android/crashreport/CrashReporter_linux.cpp
        android/emulation/dynlink_cuda.cpp
        android/emulation/dynlink_nvcuvid.cpp
        android/emulation/MediaCudaDriverHelper.cpp
        android/emulation/MediaCudaUtils.cpp
        android/emulation/MediaCudaVideoHelper.cpp
        android/opengl/NativeGpuInfo_linux.cpp
        android/snapshot/MemoryWatch_linux.cpp
  DARWIN # cmake-format: sortable
         android/camera/camera-capture-mac.m
         android/crashreport/CrashReporter_darwin.cpp
         android/emulation/MediaH264DecoderVideoToolBox.cpp
         android/emulation/MediaVideoToolBoxUtils.cpp
         android/emulation/MediaVideoToolBoxVideoHelper.cpp
         android/opengl/macTouchOpenGL.m
         android/opengl/NativeGpuInfo_darwin.cpp
         android/snapshot/MacSegvHandler.cpp
         android/snapshot/MemoryWatch_darwin.cpp)

# Note that all these dependencies will propagate to whoever relies on android-
# emu It will also setup the proper include directories, so that android-emu can
# find all the headers needed for using the library defined below. We ideally
# would like to keep this list small.
target_link_libraries(
  android-emu
  PUBLIC FFMPEG::FFMPEG
         VPX::VPX
         emulator-libext4_utils
         android-files
         android-metrics
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
         android-curl
         picosha2
         # Protobuf dependencies
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
         android-net
         android-emu-base
         breakpad_client
         ssl
         crypto
         LibXml2::LibXml2
         png
         lz4
         zlib
         android-hw-config
         android-emu-agents
         absl::strings)

target_link_libraries(android-emu PRIVATE hostapd)

if(NOT OPTION_GFXSTREAM_BACKEND)
  target_link_libraries(android-emu PRIVATE modem_simulator_lib gnss_proxy_lib)
  target_link_libraries(android-emu PUBLIC modem_simulator_lib gnss_proxy_lib)
endif()

# Here are the windows library and link dependencies. They are public and will
# propagate onwards to others that depend on android-emu
android_target_link_libraries(
  android-emu windows
  PUBLIC emulator-libmman-win32
         emulator-libusb
         setupapi::setupapi
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
android_target_link_libraries(android-emu linux-x86_64 PUBLIC -lrt -lc++)

# Here are the darwin library and link dependencies. They are public and will
# propagate onwards to others that depend on android-emu. You should really only
# add things that are crucial for this library to link
android_target_link_libraries(
  android-emu darwin-x86_64
  PUBLIC "-framework AppKit"
         "-framework AVFoundation" # For camera-capture-mac.m
         "-framework Accelerate" # Of course, our camera needs it!
         "-framework CoreMedia" # Also for the camera.
         "-framework CoreVideo" # Also for the camera.
         "-framework IOKit"
         "-framework VideoToolbox" # For HW codec acceleration on mac
         "-framework VideoDecodeAcceleration" # For HW codec acceleration on mac
         "-weak_framework Hypervisor"
         "-framework OpenGL")

android_target_link_libraries(
  android-emu darwin-aarch64
  PUBLIC "-framework AppKit"
         "-framework AVFoundation" # For camera-capture-mac.m
         "-framework Accelerate" # Of course, our camera needs it!
         "-framework CoreMedia" # Also for the camera.
         "-framework CoreVideo" # Also for the camera.
         "-framework IOKit"
         "-framework VideoToolbox" # For HW codec acceleration on mac
         "-framework VideoDecodeAcceleration" # For HW codec acceleration on mac
         "-weak_framework Hypervisor"
         "-framework OpenGL")

target_include_directories(
  android-emu
  PUBLIC
    # TODO(jansene): The next 2 imply a link dependendency on emugl libs, which
    # we have not yet made explicit
    ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/host/include
    ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/shared
    # TODO(jansene): We actually have a hard dependency on qemu-glue as there
    # are a lot of externs that are actually defined in qemu2-glue. this has to
    # be sorted out,
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG}
    ${ANDROID_QEMU2_TOP_DIR}
    # If you use our library, you get access to our headers.
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DARWINN_INCLUDE_DIRS})

android_target_compile_options(
  android-emu Clang PRIVATE -Wno-invalid-constexpr -Wno-return-type-c-linkage
                            -fvisibility=default)

android_target_compile_options(
  android-emu Clang
  PUBLIC -Wno-extern-c-compat # Needed for serial_line.h
         -Wno-return-type-c-linkage # android_getOpenglesRenderer
)

android_target_compile_options(
  android-emu linux-x86_64 PRIVATE -idirafter
                                   ${ANDROID_QEMU2_TOP_DIR}/linux-headers)
android_target_compile_options(
  android-emu darwin-x86_64
  PRIVATE -Wno-objc-method-access -Wno-missing-selector-name -Wno-receiver-expr
          -Wno-incomplete-implementation -Wno-incompatible-pointer-types
          -Wno-deprecated-declarations)

android_target_compile_options(
  android-emu windows_msvc-x86_64
  PRIVATE -Wno-unused-private-field -Wno-reorder -Wno-missing-braces
          -Wno-pessimizing-move -Wno-unused-lambda-capture)

android_target_compile_definitions(
  android-emu darwin-x86_64 PRIVATE "-D_DARWIN_C_SOURCE=1" "-Dftello64=ftell"
                                    "-Dfseeko64=fseek")
android_target_compile_definitions(
  android-emu darwin-aarch64 PRIVATE "-D_DARWIN_C_SOURCE=1" "-Dftello64=ftell"
                                     "-Dfseeko64=fseek")

target_compile_definitions(
  android-emu
  PRIVATE "-DCRASHUPLOAD=${OPTION_CRASHUPLOAD}"
          "-DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}"
          "-DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}")

if(WEBRTC)
  target_compile_definitions(android-emu PUBLIC -DANDROID_WEBRTC)
endif()

if(BLUETOOTH_EMULATION)
  target_compile_definitions(android-emu PUBLIC -DANDROID_BLUETOOTH)
endif()

if(OPTION_GFXSTREAM_BACKEND)
  target_compile_definitions(android-emu PUBLIC -DAEMU_GFXSTREAM_BACKEND=1)
endif()

# The dependent target os specific sources, they are pretty much the same as
# above, excluding camera support, because that brings in a whole slew of
# dependencies Shared version of the library. Note that this only has the set of
# common sources, otherwise you will get a lot of linker errors.

android_add_library(
  TARGET android-emu-shared
  SHARED
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      android/avd/hw-config.c
      android/avd/info.c
      android/avd/util.c
      android/avd/util_wrapper.cpp
      android/base/async/CallbackRegistry.cpp
      android/cmdline-option.cpp
      android/emulation/address_space_device.cpp
      android/emulation/address_space_graphics.cpp
      android/emulation/address_space_host_memory_allocator.cpp
      android/emulation/address_space_shared_slots_host_memory_allocator.cpp
      android/emulation/android_pipe_host.cpp
      android/emulation/AndroidAsyncMessagePipe.cpp
      android/emulation/AndroidMessagePipe.cpp
      android/emulation/AndroidPipe.cpp
      android/emulation/AudioCaptureEngine.cpp
      android/emulation/AudioOutputEngine.cpp
      android/emulation/bufprint_config_dirs.cpp
      android/emulation/ClipboardPipe.cpp
      android/emulation/ComponentVersion.cpp
      android/emulation/control/FilePusher.cpp
      android/emulation/control/LineConsumer.cpp
      android/emulation/control/NopRtcBridge.cpp
      android/emulation/DmaMap.cpp
      android/emulation/goldfish_sync.cpp
      android/emulation/GoldfishDma.cpp
      android/emulation/GoldfishSyncCommandQueue.cpp
      android/emulation/hostdevices/HostAddressSpace.cpp
      android/emulation/hostdevices/HostGoldfishPipe.cpp
      android/emulation/HostmemIdMapping.cpp
      android/emulation/LogcatPipe.cpp
      android/emulation/nand_limits.c
      android/emulation/ParameterList.cpp
      android/emulation/RefcountPipe.cpp
      android/emulation/serial_line.cpp
      android/emulation/SerialLine.cpp
      android/emulation/SetupParameters.cpp
      android/emulation/testing/TestVmLock.cpp
      android/emulation/VmLock.cpp
      android/error-messages.cpp
      android/featurecontrol/feature_control.cpp
      android/featurecontrol/FeatureControl.cpp
      android/featurecontrol/FeatureControlImpl.cpp
      android/framebuffer.c
      android/gps.c
      android/gpu_frame.cpp
      android/hw-events.c
      android/hw-kmsg.c
      android/hw-lcd.c
      android/kernel/kernel_utils.cpp
      android/loadpng.c
      android/opengl/emugl_config.cpp
      android/opengl/EmuglBackendList.cpp
      android/opengl/EmuglBackendScanner.cpp
      android/opengl/GLProcessPipe.cpp
      android/opengl/GpuFrameBridge.cpp
      android/opengl/gpuinfo.cpp
      android/opengl/logger.cpp
      android/opengl/OpenglEsPipe.cpp
      android/opengles.cpp
      android/protobuf/LoadSave.cpp
      android/snaphost-android.c
      android/snapshot.c
      android/snapshot/common.cpp
      android/snapshot/Compressor.cpp
      android/snapshot/Decompressor.cpp
      android/snapshot/GapTracker.cpp
      android/snapshot/Hierarchy.cpp
      android/snapshot/IncrementalStats.cpp
      android/snapshot/interface.cpp
      android/snapshot/Loader.cpp
      android/snapshot/MemoryWatch_common.cpp
      android/snapshot/PathUtils.cpp
      android/snapshot/Quickboot.cpp
      android/snapshot/RamLoader.cpp
      android/snapshot/RamSaver.cpp
      android/snapshot/RamSnapshotTesting.cpp
      android/snapshot/Saver.cpp
      android/snapshot/Snapshot.cpp
      android/snapshot/Snapshotter.cpp
      android/snapshot/TextureLoader.cpp
      android/snapshot/TextureSaver.cpp
      android/user-config.cpp
      android/utils/Random.cpp
      stubs/gfxstream-stubs.cpp
      stubs/stubs.cpp
  WINDOWS # cmake-format: sortable
          android/emulation/dynlink_cuda.cpp
          android/emulation/dynlink_nvcuvid.cpp
          android/opengl/NativeGpuInfo_windows.cpp
          android/snapshot/MemoryWatch_windows.cpp
          android/windows_installer.cpp
  LINUX # cmake-format: sortable
        android/emulation/dynlink_cuda.cpp android/emulation/dynlink_nvcuvid.cpp
        android/opengl/NativeGpuInfo_linux.cpp
        android/snapshot/MemoryWatch_linux.cpp
  DARWIN # cmake-format: sortable
         android/opengl/macTouchOpenGL.m
         android/opengl/NativeGpuInfo_darwin.cpp
         android/snapshot/MacSegvHandler.cpp
         android/snapshot/MemoryWatch_darwin.cpp)

# Note that these are basically the same as android-emu-shared. We should clean
# this up
target_link_libraries(
  android-emu-shared
  PUBLIC android-emu-base
         android-emu-agents
         emulator-murmurhash
         android-files
         android-metrics
         # Protobuf dependencies
         snapshot
         # Prebuilt libraries
         png
         lz4
         zlib
         android-hw-config
         absl::strings)
# Here are the windows library and link dependencies. They are public and will
# propagate onwards to others that depend on android-emu-shared
android_target_link_libraries(
  android-emu-shared windows
  PRIVATE emulator-libmman-win32
          emulator-libusb
          setupapi::setupapi
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
          iphlpapi::iphlpapi)
# These are the libs needed for android-emu-shared on linux.
android_target_link_libraries(android-emu-shared linux-x86_64 PRIVATE -lrt)
# Here are the darwin library and link dependencies. They are public and will
# propagate onwards to others that depend on android-emu-shared. You should
# really only add things that are crucial for this library to link If you don't
# you might see bizarre errors. (Add opengl as a link dependency, you will have
# fun)
android_target_link_libraries(
  android-emu-shared darwin-x86_64
  PRIVATE "-framework AppKit"
          "-framework ApplicationServices" # To control icon
          "-framework AVFoundation" # For camera-capture-mac.m
          "-framework Accelerate" # Of course, our camera needs it!
          "-framework CoreMedia" # Also for the camera.
          "-framework CoreVideo" # Also for the camera.
          "-framework VideoToolbox" # For HW codec acceleration on mac
          "-framework VideoDecodeAcceleration" # For HW codec acceleration on
                                               # mac
          "-framework OpenGL"
          "-framework IOKit")

android_target_link_libraries(
  android-emu-shared darwin-aarch64
  PRIVATE "-framework AppKit"
          "-framework ApplicationServices" # To control icon
          "-framework AVFoundation" # For camera-capture-mac.m
          "-framework Accelerate" # Of course, our camera needs it!
          "-framework CoreMedia" # Also for the camera.
          "-framework CoreVideo" # Also for the camera.
          "-framework VideoToolbox" # For HW codec acceleration on mac
          "-framework VideoDecodeAcceleration" # For HW codec acceleration on
                                               # mac
          "-framework OpenGL"
          "-framework IOKit")
target_include_directories(
  android-emu-shared
  PUBLIC
    # TODO(jansene): The next 2 imply a link dependendency on emugl libs, which
    # we have not yet made explicit
    ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/host/include
    ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/shared
    # TODO(jansene): We actually have a hard dependency on qemu-glue as there
    # are a lot of externs that are actually defined in qemu2-glue. this has to
    # be sorted out,
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG}
    # If you use our library, you get access to our headers.
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})
android_target_compile_options(
  android-emu-shared Clang PRIVATE -Wno-extern-c-compat -Wno-invalid-constexpr
                                   -fvisibility=default)
android_target_compile_options(
  android-emu-shared Clang PUBLIC -Wno-return-type-c-linkage) # android_getOp
# enG lesRenderer
android_target_compile_options(
  android-emu-shared linux-x86_64
  PRIVATE -idirafter ${ANDROID_QEMU2_TOP_DIR}/linux-headers)
android_target_compile_options(
  android-emu-shared darwin-x86_64
  PRIVATE -Wno-error -Wno-objc-method-access -Wno-receiver-expr
          -Wno-incomplete-implementation -Wno-missing-selector-name
          -Wno-incompatible-pointer-types)
android_target_compile_options(
  android-emu-shared windows_msvc-x86_64
  PRIVATE -Wno-unused-private-field -Wno-reorder -Wno-unused-lambda-capture)
# Definitions needed to compile our deps as static
target_compile_definitions(android-emu-shared PUBLIC ${CURL_DEFINITIONS}
                                                     ${LIBXML2_DEFINITIONS})
android_target_compile_definitions(
  android-emu-shared darwin-x86_64
  PRIVATE "-D_DARWIN_C_SOURCE=1" "-Dftello64=ftell" "-Dfseeko64=fseek")
android_target_compile_definitions(
  android-emu-shared darwin-aarch64
  PRIVATE "-D_DARWIN_C_SOURCE=1" "-Dftello64=ftell" "-Dfseeko64=fseek")
target_compile_definitions(
  android-emu-shared
  PRIVATE "-DCRASHUPLOAD=${OPTION_CRASHUPLOAD}"
          "-DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}"
          "-DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}")
if(WEBRTC)
  target_compile_definitions(android-emu-shared PUBLIC -DANDROID_WEBRTC)
endif()

target_compile_definitions(android-emu-shared PUBLIC -DAEMU_MIN=1)

if(OPTION_GFXSTREAM_BACKEND)
  target_compile_definitions(android-emu-shared
                             PUBLIC -DAEMU_GFXSTREAM_BACKEND=1)
  android_install_shared(android-emu-shared)
endif()

# This library contains a main entry point that injects fake console agents into
# your unit tests. you usually want to link against this library if you need to
# make any calls to getConsoleAgents()
android_add_library(
  TARGET android-emu-test-launcher
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      android/emulation/testing/MockAndroidAgentFactory.cpp
      android/emulation/testing/MockAndroidEmulatorWindowAgent.cpp
      android/emulation/testing/MockAndroidMultiDisplayAgent.cpp
      android/emulation/testing/MockAndroidVmOperations.cpp)
android_target_compile_options(android-emu-test-launcher Clang
                               PRIVATE -O0 -Wno-invalid-constexpr)
target_link_libraries(android-emu-test-launcher PRIVATE android-emu
                      PUBLIC gmock)

if(NOT LINUX_AARCH64)
  set(android-emu_unittests_common
      # cmake-format: sortable
      android/automation/AutomationController_unittest.cpp
      android/automation/AutomationEventSink_unittest.cpp
      android/avd/util_unittest.cpp
      android/avd/util_wrapper_unittest.cpp
      # bug: 153381599: disabled until flakiness is addressed
      android/base/async/CallbackRegistry_unittest.cpp
      android/base/IOVector_unittest.cpp
      android/base/LayoutResolver_unittest.cpp
      android/base/testing/ProtobufMatchers.cpp
      android/bootconfig_unittest.cpp
      android/camera/CameraFormatConverters_unittest.cpp
      android/cmdline-option_unittest.cpp
      android/CommonReportedInfo_unittest.cpp
      android/console_auth_unittest.cpp
      android/console_unittest.cpp
      android/emulation/AdbDebugPipe_unittest.cpp
      android/emulation/AdbGuestPipe_unittest.cpp
      android/emulation/AdbHostListener_unittest.cpp
      android/emulation/AdbHostServer_unittest.cpp
      android/emulation/AdbHub_unittest.cpp
      android/emulation/AdbMessageSniffer_unittest.cpp
      android/emulation/address_space_graphics_unittests.cpp
      android/emulation/address_space_host_memory_allocator_unittests.cpp
      android/emulation/address_space_shared_slots_host_memory_allocator_unittests.cpp
      android/emulation/android_pipe_pingpong_unittest.cpp
      android/emulation/android_pipe_zero_unittest.cpp
      android/emulation/AndroidAsyncMessagePipe_unittest.cpp
      android/emulation/bufprint_config_dirs_unittest.cpp
      android/emulation/ComponentVersion_unittest.cpp
      android/emulation/control/adb/AdbConnection_unittest.cpp
      android/emulation/control/adb/AdbInterface_unittest.cpp
      android/emulation/control/adb/adbkey_unittest.cpp
      android/emulation/control/ApkInstaller_unittest.cpp
      android/emulation/control/EmulatorAdvertisement_unittest.cpp
      android/emulation/control/FilePusher_unittest.cpp
      android/emulation/control/GooglePlayServices_unittest.cpp
      android/emulation/control/LineConsumer_unittest.cpp
      android/emulation/control/ScreenCapturer_unittest.cpp
      android/emulation/CpuAccelerator_unittest.cpp
      android/emulation/CrossSessionSocket_unittest.cpp
      android/emulation/DeviceContextRunner_unittest.cpp
      android/emulation/DmaMap_unittest.cpp
      android/emulation/hostdevices/HostAddressSpace_unittest.cpp
      android/emulation/hostdevices/HostGoldfishPipe_unittest.cpp
      android/emulation/HostmemIdMapping_unittest.cpp
      android/emulation/HostMemoryService_unittest.cpp
      android/emulation/Hypervisor_unittest.cpp
      android/emulation/ParameterList_unittest.cpp
      android/emulation/RefcountPipe_unittest.cpp
      android/emulation/serial_line_unittest.cpp
      android/emulation/SetupParameters_unittest.cpp
      android/emulation/testing/TestAndroidPipeDevice.cpp
      android/emulation/VmLock_unittest.cpp
      android/error-messages_unittest.cpp
      android/featurecontrol/FeatureControl_unittest.cpp
      android/featurecontrol/HWMatching_unittest.cpp
      android/featurecontrol/testing/FeatureControlTest.cpp
      android/gps/GpxParser_unittest.cpp
      android/gps/KmlParser_unittest.cpp
      android/hw-lcd_unittest.cpp
      android/jdwp/Jdwp_unittest.cpp
      android/jdwp/JdwpProxy_unittest.cpp
      android/kernel/kernel_utils_unittest.cpp
      android/location/MapsKey_unittest.cpp
      android/location/MapsKeyFileParser_unittest.cpp
      android/location/Point_unittest.cpp
      android/location/Route_unittest.cpp
      android/network/constants_unittest.cpp
      android/network/control_unittest.cpp
      android/network/GenericNetlinkMessage_unittest.cpp
      android/network/Ieee80211Frame_unittest.cpp
      android/network/MacAddress_unittest.cpp
      android/network/WifiForwardPeer_unittest.cpp
      android/offworld/OffworldPipe_unittest.cpp
      android/opengl/emugl_config_unittest.cpp
      android/opengl/EmuglBackendList_unittest.cpp
      android/opengl/EmuglBackendScanner_unittest.cpp
      android/opengl/GpuFrameBridge_unittest.cpp
      android/opengl/gpuinfo_unittest.cpp
      android/physics/AmbientEnvironment_unittest.cpp
      android/physics/BodyModel_unittest.cpp
      android/physics/InertialModel_unittest.cpp
      android/physics/PhysicalModel_unittest.cpp
      android/proxy/proxy_common_unittest.cpp
      android/proxy/ProxyUtils_unittest.cpp
      android/sensor_replay/sensor_session_playback_unittest.cpp
      android/snapshot/RamLoader_unittest.cpp
      android/snapshot/RamSaver_unittest.cpp
      android/snapshot/RamSnapshot_unittest.cpp
      android/snapshot/Snapshot_unittest.cpp
      android/telephony/gsm_unittest.cpp
      android/telephony/modem_unittest.cpp
      android/telephony/SimAccessRules_unittest.cpp
      android/telephony/sms_unittest.cpp
      android/telephony/TagLengthValue_unittest.cpp
      android/update-check/UpdateChecker_unittest.cpp
      android/update-check/VersionExtractor_unittest.cpp
      android/userspace-boot-properties_unittest.cpp
      android/utils/dns_unittest.cpp
      android/utils/format_unittest.cpp
      android/utils/host_bitness_unittest.cpp
      android/utils/path_unittest.cpp
      android/utils/Random_unittest.cpp
      android/utils/sockets_unittest.cpp
      android/verified-boot/load_config_unittest.cpp
      android/videoinjection/VideoInjectionController_unittest.cpp
      android/virtualscene/TextureUtils_unittest.cpp
      android/wear-agent/PairUpWearPhone_unittest.cpp
      android/wear-agent/testing/WearAgentTestUtils.cpp
      android/wear-agent/WearAgent_unittest.cpp)

  # And declare the test
  android_add_test(
    TARGET android-emu_unittests
    SRC # cmake-format: sortable
        ${android-emu_unittests_common}
    WINDOWS # cmake-format: sortable
            android/utils/win32_cmdline_quote_unittest.cpp
            android/windows_installer_unittest.cpp
    DARWIN # cmake-format: sortable
           android/emulation/control/adb/AdbShellStream_unittest.cpp
           android/emulation/nand_limits_unittest.cpp
    LINUX # cmake-format: sortable
          android/emulation/control/adb/AdbShellStream_unittest.cpp
          android/emulation/nand_limits_unittest.cpp)
  # Setup the targets compile config etc..
  android_target_compile_options(
    android-emu_unittests Clang PRIVATE -O0 -Wno-invalid-constexpr
                                        -Wno-string-plus-int)
  target_include_directories(android-emu_unittests
                             PRIVATE ../android-emugl/host/include/)

  # Sign unit test if needed.
  android_sign(TARGET android-emu_unittests)

  # Settings needed for darwin
  android_target_compile_definitions(
    android-emu_unittests darwin-x86_64
    PRIVATE "-D_DARWIN_C_SOURCE=1" "-Dftello64=ftell" "-Dfseeko64=fseek")
  android_target_compile_options(android-emu_unittests darwin-x86_64
                                 PRIVATE "-Wno-deprecated-declarations")
  android_target_compile_definitions(
    android-emu_unittests darwin-aarch64
    PRIVATE "-D_DARWIN_C_SOURCE=1" "-Dftello64=ftell" "-Dfseeko64=fseek")
  android_target_compile_options(android-emu_unittests darwin-aarch64
                                 PRIVATE "-Wno-deprecated-declarations")

  # Dependecies are exported from android-emu.
  target_link_libraries(android-emu_unittests PRIVATE android-emu
                                                      android-emu-test-launcher)

  android_add_executable(
    NODISTRIBUTE TARGET studio_discovery_tester
    SRC # cmake-format: sortable
        android/emulation/control/StudioDiscoveryTester.cpp)
  target_link_libraries(studio_discovery_tester PRIVATE android-grpc
                                                        android-emu)
  add_dependencies(android-emu_unittests studio_discovery_tester)

  list(
    APPEND
    # cmake-format: sortable
    android-emu-testdata
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
  android_copy_test_files(android-emu_unittests "${android-emu-testdata}"
                          testdata)
  android_target_dependency(android-emu_unittests all
                            VIRTUAL_SCENE_DEPENDENCIES)
  android_copy_test_dir(android-emu_unittests test-sdk test-sdk)
  android_copy_file(
    android-emu_unittests
    "${CMAKE_CURRENT_SOURCE_DIR}/android/emulation/CpuAccelerator_unittest.dat"
    "$<TARGET_FILE_DIR:android-emu_unittests>/android/android-emu/android/emulation/CpuAccelerator_unittest.dat"
  )
  android_copy_file(
    android-emu_unittests
    "${CMAKE_CURRENT_SOURCE_DIR}/android/emulation/CpuAccelerator_unittest.dat2"
    "$<TARGET_FILE_DIR:android-emu_unittests>/android/android-emu/android/emulation/CpuAccelerator_unittest.dat2"
  )
endif()
