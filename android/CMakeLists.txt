file(COPY ${CMAKE_CURRENT_LIST_DIR}/data/ca-bundle.pem DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data)
file(COPY ${CMAKE_CURRENT_LIST_DIR}/data/advancedFeatures.ini DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data)

# android-emu-base

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(ANDROID_EMU_BASE_PLATFORM_INCLUDE
        ${REPO_PREBUILTS_AEB}/common/e2fsprogs/${BUILD_TARGET_TAG}/include
        )
    set(ANDROID_EMU_BASE_PLATFORM_LIB
        ${REPO_PREBUILTS_AEB}/common/e2fsprogs/${BUILD_TARGET_TAG}/lib/libuuid.a
        )
    set(ANDROID_EMU_BASE_PLATFORM_SRC
        base/threads/Thread_pthread.cpp
        )
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(ANDROID_EMU_BASE_PLATFORM_SRC
        base/threads/Thread_pthread.cpp
        )
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(ANDROID_EMU_BASE_PLATFORM_LIB
        -lrpcrt4  # For uuid functions
        -lpsapi  # For GetPerformanceInfo in CrashService_windows.cpp
        )
    set(ANDROID_EMU_BASE_PLATFORM_SRC
        base/synchronization/ConditionVariable_win32.cpp
        base/synchronization/ReadWriteLock_win32.cpp
        base/threads/Thread_win32.cpp
        base/system/Win32Utils.cpp
        base/system/Win32UnicodeString.cpp
        utils/win32_cmdline_quote.cpp
        utils/win32_unicode.cpp
        )
endif()

add_library(android-emu-base STATIC
    ${ANDROID_EMU_BASE_PLATFORM_SRC}
    base/containers/PointerSet.cpp
    base/containers/HashUtils.cpp
    base/files/PathUtils.cpp
    base/files/StdioStream.cpp
    base/files/Stream.cpp
    base/files/IniFile.cpp
    base/misc/FileUtils.cpp
    base/misc/HttpUtils.cpp
    base/misc/StringUtils.cpp
    base/misc/Utf8Utils.cpp
    base/StringFormat.cpp
    base/StringView.cpp
    base/sockets/SocketUtils.cpp
    base/sockets/SocketWaiter.cpp
    base/synchronization/MessageChannel.cpp
    base/Log.cpp
    base/memory/LazyInstance.cpp
    base/system/System.cpp
    base/threads/Async.cpp
    base/threads/FunctorThread.cpp
    base/threads/ThreadStore.cpp
    base/Uri.cpp
    base/Uuid.cpp
    base/Version.cpp
    utils/aconfig-file.c
    utils/assert.c
    utils/async.cpp
    utils/bufprint.c
    utils/bufprint_system.cpp
    utils/cbuffer.c
    utils/debug.c
    utils/debug_wrapper.cpp
    utils/dns.cpp
    utils/dll.c
    utils/dirscanner.cpp
    utils/eintr_wrapper.c
    utils/exec.cpp
    utils/filelock.c
    utils/file_data.c
    utils/file_io.cpp
    utils/format.cpp
    utils/host_bitness.cpp
    utils/http_utils.cpp
    utils/iolooper.cpp
    utils/ini.cpp
    utils/intmap.c
    utils/ipaddr.cpp
    utils/lineinput.c
    utils/lock.cpp
    utils/mapfile.c
    utils/misc.c
    utils/panic.c
    utils/path.cpp
    utils/path_system.cpp
    utils/property_file.c
    utils/reflist.c
    utils/refset.c
    utils/sockets.c
    utils/stralloc.c
    utils/stream.cpp
    utils/string.cpp
    utils/system.c
    utils/system_wrapper.cpp
    utils/tempfile.c
    utils/timezone.cpp
    utils/uri.cpp
    utils/utf8_utils.cpp
    utils/vector.c
    utils/x86_cpuid.cpp
    )

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DLOG 1)
else()
    set(DLOG 0)
endif()

if(NOT EMU_CRASHUPLOAD)
    set(EMU_CRASHUPLOAD NONE)
endif()

if(ANDROID_SDK_TOOLS_REVISION)
    set(ANDROID_EMU_BASE_OPT -DANDROID_SDK_TOOLS_REVISION=${ANDROID_SDK_TOOLS_REVISION})
endif()
if(ANDROID_SDK_TOOLS_BUILD_NUMBER)
    set(ANDROID_EMU_BASE_OPT ${ANDROID_EMU_BASE_OPT} -DANDROID_SDK_TOOLS_BUILD_NUMBER=${ANDROID_SDK_TOOLS_BUILD_NUMBER})
endif()

target_compile_options(android-emu-base
    PUBLIC
    -Werror=implicit-function-declaration
    -DENABLE_DLOG=${DLOG}
    -DCRASHUPLOAD=${EMU_CRASHUPLOAD}
    )

target_link_libraries(android-emu-base
    PUBLIC
    ${ANDROID_EMU_BASE_PLATFORM_LIB}
    )

target_include_directories(android-emu-base
    PUBLIC
    ${REPO_QEMU}
    ${ANDROID_EMU_BASE_PLATFORM_INCLUDE}
    )

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

# create config-host.h
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CONFIG_FDATASYNC 1)
    set(CONFIG_KVM_GS_RESTORE 1)
    set(CONFIG_LINUX 1)
    set(CONFIG_SIGNALFD 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CONFIG_BSD 1)
    set(CONFIG_DARWIN 1)
    set(O_LARGEFILE 0)
    set(MAP_ANONYMOUS MAP_ANON)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CONFIG_WIN32 1)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "(Linux|Darwin)")
    set(CONFIG_IOVEC 1)
    set(CONFIG_MADVISE 1)
    set(CONFIG_NAND_LIMITS 1)
    set(CONFIG_POSIX 1)
endif()

include(CheckIncludeFiles)
check_include_files(byteswap.h CONFIG_BYTESWAP_H)
check_include_files(machine/bswap.h CONFIG_MACHINE_BSWAP_H)
check_include_files(fnmatch.h CONFIG_FNMATCH)
set(ANDROID_EMU_GENERATED_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/generated/config/)
configure_file(config/config-host.h.in ${ANDROID_EMU_GENERATED_CONFIG}/config-host.h)


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(ANDROID_EMU_PLATFORM_SRC
        camera/camera-capture-linux.c
        crashreport/CrashReporter_linux.cpp
        )
    set(ANDROID_EMU_PLATFORM_LIB
        selinux
        )
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(ANDROID_EMU_PLATFORM_SRC
        camera/camera-capture-mac.m
        crashreport/CrashReporter_darwin.cpp
        )
    set(ANDROID_EMU_PLATFORM_LIB
        selinux
        )
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(ANDROID_EMU_PLATFORM_SRC
        camera/camera-capture-windows.cpp
        crashreport/CrashReporter_windows.cpp
        windows_installer.cpp
        )
    set(ANDROID_EMU_PLATFORM_OPT
        -DCURL_STATICLIB
        -DLIBXML_STATIC
        )
endif()

file(COPY ${REPO_PREBUILTS_AEB}/common/e2fsprogs/${BUILD_TARGET_TAG}/sbin/resize2fs DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# TODO(vharron) Windows, copy cygwin dlls?

set(HW_PROP_INI ${CMAKE_CURRENT_LIST_DIR}/avd/hardware-properties.ini)
set(HW_CONFIG_DEFS_H ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h)
get_filename_component(HW_CONFIG_DEFS_H_DIR ${HW_CONFIG_DEFS_H} DIRECTORY)
file(MAKE_DIRECTORY ${HW_CONFIG_DEFS_H_DIR})
add_custom_command(
    OUTPUT ${HW_CONFIG_DEFS_H}
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/tools/gen-hw-config.py ${HW_PROP_INI} ${HW_CONFIG_DEFS_H}
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/tools/gen-hw-config.py ${HW_PROP_INI}
    )

add_library(android-emu STATIC
    ${HW_CONFIG_DEFS_H}
    ${ANDROID_EMU_PLATFORM_SRC}
    ${ANDROID_EMU_GENERATED_CONFIG}/config-host.h
    adb-qemud.c
    adb-server.c
    avd/hw-config.c
    avd/info.c
    avd/scanner.c
    avd/util.c
    avd/util_wrapper.cpp
    async-console.c
    async-socket.c
    async-socket-connector.c
    async-utils.c
    base/async/AsyncReader.cpp
    base/async/AsyncSocketServer.cpp
    base/async/AsyncWriter.cpp
    base/async/Looper.cpp
    base/async/ScopedSocketWatch.cpp
    base/async/ThreadLooper.cpp
    base/sockets/SocketDrainer.cpp
    base/threads/internal/ParallelTaskBase.cpp
    boot-properties.c
    camera/camera-service.c
    camera/camera-format-converters.c
    cmdline-option.c
    console.c
    console_auth.cpp
    cpu_accelerator.cpp
    crashreport/CrashSystem.cpp
    crashreport/CrashReporter_common.cpp
    curl-support.c
    emulation/android_pipe.c
    emulation/android_pipe_pingpong.c
    emulation/android_pipe_throttle.c
    emulation/android_pipe_unix.cpp
    emulation/android_pipe_zero.c
    emulation/android_qemud.cpp
    emulation/bufprint_config_dirs.cpp
    emulation/ConfigDirs.cpp
    emulation/control/AdbInterface.cpp
    emulation/control/ApkInstaller.cpp
    emulation/control/FilePusher.cpp
    emulation/control/LineConsumer.cpp
    emulation/control/ScreenCapturer.cpp
    emulation/CpuAccelerator.cpp
    emulation/nand_limits.c
    emulation/ParameterList.cpp
    emulation/qemud/android_qemud_client.cpp
    emulation/qemud/android_qemud_multiplexer.cpp
    emulation/qemud/android_qemud_serial.cpp
    emulation/qemud/android_qemud_service.cpp
    emulation/qemud/android_qemud_sink.cpp
    emulation/serial_line.cpp
    emulation/SerialLine.cpp
    emulation/SetupParameters.cpp
    emulation/VmLock.cpp
    emulation/vm_lock.cpp
    emulator-window.c
    error-messages.cpp
    featurecontrol/FeatureControl.cpp
    featurecontrol/FeatureControlImpl.cpp
    filesystems/ext4_resize.cpp
    filesystems/ext4_utils.cpp
    filesystems/fstab_parser.cpp
    filesystems/internal/PartitionConfigBackend.cpp
    filesystems/partition_config.cpp
    filesystems/partition_types.cpp
    filesystems/ramdisk_extractor.cpp
    framebuffer.c
    gps/GpxParser.cpp
    gps/KmlParser.cpp
    gps.c
    gpu_frame.cpp
    help.c
    hw-control.c
    hw-events.c
    hw-fingerprint.c
    hw-kmsg.c
    hw-lcd.c
    hw-qemud.cpp
    hw-sensors.c
    jpeg-compress.c
    kernel/kernel_utils.cpp
    loadpng.c
    main-common.c
    main-emugl.cpp
    main-kernel-parameters.cpp
    main-qemu-parameters.cpp
    metrics/AdbLivenessChecker.cpp
    metrics/IniFileAutoFlusher.cpp
    metrics/metrics_reporter.cpp
    metrics/metrics_reporter_toolbar.cpp
    metrics/StudioConfig.cpp
    misc/Random.cpp
    multitouch-port.c
    multitouch-screen.c
    network/constants.c
    network/globals.c
    opengl/EmuglBackendList.cpp
    opengl/EmuglBackendScanner.cpp
    opengl/emugl_config.cpp
    opengl/GpuFrameBridge.cpp
    opengl/gpuinfo.cpp
    opengl/logger.cpp
    opengl/OpenglEsPipe.cpp
    opengles.cpp
    openssl-support.cpp
    process_setup.cpp
    proxy/proxy_common.c
    proxy/proxy_http.c
    proxy/proxy_http_connector.c
    proxy/proxy_http_rewriter.c
    qemu-setup.c
    qemu-tcpdump.c
    resource.c
    sdk-controller-socket.c
    sensors-port.c
    shaper.c
    snaphost-android.c
    snapshot.c
    telephony/debug.c
    telephony/gsm.c
    telephony/modem.c
    telephony/modem_driver.c
    telephony/remote_call.c
    telephony/sim_card.c
    telephony/sms.c
    telephony/sysdeps.c
    uncompress.cpp
    update-check/UpdateChecker.cpp
    update-check/VersionExtractor.cpp
    user-config.cpp
    utils/socket_drainer.cpp
    utils/looper.cpp
    wear-agent/android_wear_agent.cpp
    wear-agent/WearAgent.cpp
    wear-agent/PairUpWearPhone.cpp
    )

target_compile_options(android-emu
    PUBLIC
    ${ANDROID_EMU_PLATFORM_OPT}
    )

target_link_libraries(android-emu
    PUBLIC
    android-emu-base
    emugl_common
    jpeg
    ${ANDROID_EMU_PLATFORM_LIB}
    ext4_utils
    -L${REPO_PREBUILTS_AEB}/common/breakpad/${BUILD_TARGET_TAG}/lib
    breakpad_client
    disasm
    breakpad
    -L${REPO_PREBUILTS_AEB}/curl/${BUILD_TARGET_TAG}/lib
    curl
    ssl
    crypto
    sparse
    ${REPO_PREBUILTS_AEB}/common/libxml2/${BUILD_TARGET_TAG}/lib/libxml2.a
    )

target_include_directories(android-emu
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ANDROID_EMU_GENERATED_CONFIG}
    PRIVATE
    ${REPO_PREBUILTS_AEB}/common/breakpad/${BUILD_TARGET_TAG}/include/breakpad
    ${REPO_PREBUILTS_AEB}/curl/${BUILD_TARGET_TAG}/include
    ${REPO_PREBUILTS_AEB}/common/libxml2/${BUILD_TARGET_TAG}/include
    )

################################################################################
# android_emu_unittests

if(CMAKE_SYSTEM_NAME MATCHES "(Linux|Darwin)")
    set(ANDROID_EMU_UNITTEST_PLATFORM_SRC
        emulation/nand_limits_unittest.cpp
        )
else()
    set(ANDROID_EMU_UNITTEST_PLATFORM_SRC
        base/files/ScopedFileHandle_unittest.cpp
        base/files/ScopedRegKey_unittest.cpp
        base/system/Win32UnicodeString_unittest.cpp
        base/system/Win32Utils_unittest.cpp
        utils/win32_cmdline_quote_unittest.cpp
        windows_installer_unittest.cpp
        )
endif()

add_executable(android_emu_unittests
    ${ANDROID_EMU_UNITTEST_PLATFORM_SRC}
    avd/util_unittest.cpp
    base/ArraySize_unittest.cpp
    base/async/Looper_unittest.cpp
    base/async/AsyncSocketServer_unittest.cpp
    base/async/RecurrentTask_unittest.cpp
    base/async/ScopedSocketWatch_unittest.cpp
    base/async/SubscriberList_unittest.cpp
    base/containers/CircularBuffer_unittest.cpp
    base/containers/HashUtils_unittest.cpp
    base/containers/Lookup_unittest.cpp
    base/containers/PointerSet_unittest.cpp
    base/containers/ScopedPointerSet_unittest.cpp
    base/containers/TailQueueList_unittest.cpp
    base/EintrWrapper_unittest.cpp
    base/files/IniFile_unittest.cpp
    base/files/PathUtils_unittest.cpp
    base/files/ScopedFd_unittest.cpp
    base/files/ScopedStdioFile_unittest.cpp
    base/files/Stream_unittest.cpp
    base/Log_unittest.cpp
    base/memory/LazyInstance_unittest.cpp
    base/memory/MallocUsableSize_unittest.cpp
    base/memory/QSort_unittest.cpp
    base/misc/FileUtils_unittest.cpp
    base/misc/HttpUtils_unittest.cpp
    base/misc/StringUtils_unittest.cpp
    base/misc/Utf8Utils_unittest.cpp
    base/Optional_unittest.cpp
    base/sockets/ScopedSocket_unittest.cpp
    base/sockets/SocketDrainer_unittest.cpp
    base/sockets/SocketUtils_unittest.cpp
    base/sockets/SocketWaiter_unittest.cpp
    base/StringFormat_unittest.cpp
    base/StringView_unittest.cpp
    base/synchronization/Lock_unittest.cpp
    base/synchronization/ReadWriteLock_unittest.cpp
    base/synchronization/MessageChannel_unittest.cpp
    base/system/System_unittest.cpp
    base/threads/FunctorThread_unittest.cpp
    base/threads/ParallelTask_unittest.cpp
    base/threads/Thread_unittest.cpp
    base/threads/ThreadStore_unittest.cpp
    base/TypeUtils_unittest.cpp
    base/Uri_unittest.cpp
    base/Uuid_unittest.cpp
    base/Version_unittest.cpp
    cmdline-option_unittest.cpp
    console_auth_unittest.cpp
    emulation/android_pipe_pingpong_unittest.cpp
    emulation/android_pipe_zero_unittest.cpp
    emulation/bufprint_config_dirs_unittest.cpp
    emulation/ConfigDirs_unittest.cpp
    emulation/control/AdbInterface_unittest.cpp
    emulation/control/ApkInstaller_unittest.cpp
    emulation/control/FilePusher_unittest.cpp
    emulation/control/ScreenCapturer_unittest.cpp
    emulation/control/LineConsumer_unittest.cpp
    emulation/CpuAccelerator_unittest.cpp
    emulation/ParameterList_unittest.cpp
    emulation/serial_line_unittest.cpp
    emulation/SetupParameters_unittest.cpp
    emulation/testing/TestAndroidPipeDevice.cpp
    emulation/VmLock_unittest.cpp
    emulation/vm_lock_unittest.cpp
    error-messages_unittest.cpp
    featurecontrol/FeatureControl_unittest.cpp
    filesystems/ext4_resize_unittest.cpp
    filesystems/ext4_utils_unittest.cpp
    filesystems/fstab_parser_unittest.cpp
    filesystems/partition_config_unittest.cpp
    filesystems/partition_types_unittest.cpp
    filesystems/ramdisk_extractor_unittest.cpp
    filesystems/testing/TestSupport.cpp
    gps/GpxParser_unittest.cpp
    gps/KmlParser_unittest.cpp
    kernel/kernel_utils_unittest.cpp
    metrics/metrics_reporter_unittest.cpp
    metrics/metrics_reporter_toolbar_unittest.cpp
    metrics/StudioConfig_unittest.cpp
    misc/Random_unittest.cpp
    network/constants_unittest.cpp
    opengl/EmuglBackendList_unittest.cpp
    opengl/EmuglBackendScanner_unittest.cpp
    opengl/emugl_config_unittest.cpp
    opengl/GpuFrameBridge_unittest.cpp
    opengl/gpuinfo_unittest.cpp
    proxy/proxy_common_unittest.cpp
    telephony/gsm_unittest.cpp
    telephony/sms_unittest.cpp
    update-check/UpdateChecker_unittest.cpp
    update-check/VersionExtractor_unittest.cpp
    utils/aconfig-file_unittest.cpp
    utils/bufprint_unittest.cpp
    utils/dirscanner_unittest.cpp
    utils/eintr_wrapper_unittest.cpp
    utils/file_data_unittest.cpp
    utils/format_unittest.cpp
    utils/host_bitness_unittest.cpp
    utils/path_unittest.cpp
    utils/property_file_unittest.cpp
    utils/string_unittest.cpp
    utils/sockets_unittest.cpp
    utils/x86_cpuid_unittest.cpp
    wear-agent/PairUpWearPhone_unittest.cpp
    wear-agent/testing/WearAgentTestUtils.cpp
    wear-agent/WearAgent_unittest.cpp
    )

target_link_libraries(android_emu_unittests
    android-emu
    gtest
    )

target_compile_options(android_emu_unittests
    PRIVATE
    -O0
    )

################################################################################
# emulator-ui
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(EMULATOR_UI_PLATFORM_SRC
        skin/qt/mac-native-window.mm
        )
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(EMULATOR_UI_PLATFORM_SRC
        skin/qt/windows-native-window.cpp
        )
endif()

# ui_generate_headers
# compile specified *.ui files -> *.h files
# RETURN_VALUE - set of generated *.h files
function(ui_generate_headers RETURN_VALUE)
    set(QT_TOP ${REPO_PREBUILTS_AEB}/qt/${BUILD_TARGET_TAG})

    foreach(UI_FILE ${ARGN})
        get_filename_component(UI_BASENAME ${UI_FILE} NAME_WE)
        # do something interesting with file 'f'
        set(UIC_IN ${CMAKE_CURRENT_LIST_DIR}/${UI_FILE})
        set(UIC_OUT ${CMAKE_CURRENT_BINARY_DIR}/ui_${UI_BASENAME}.h)
        add_custom_command(
            OUTPUT ${UIC_OUT}
            COMMAND ${CMAKE_CURRENT_LIST_DIR}/scripts/ld_library_run.py ${QT_TOP}/lib ${QT_TOP}/bin/uic -o ${UIC_OUT} ${UIC_IN}
            DEPENDS ${QT_TOP}/bin/uic ${UIC_IN}
            )
        set(HEADER_LIST ${HEADER_LIST} ${UIC_OUT})
    endforeach()
    set(${RETURN_VALUE} ${HEADER_LIST} PARENT_SCOPE)
endfunction()

ui_generate_headers(
    EMULATOR_UI_UIC_SRC
    skin/qt/extended.ui
    skin/qt/extended-pages/battery-page.ui
    skin/qt/extended-pages/cellular-page.ui
    skin/qt/extended-pages/dpad-page.ui
    skin/qt/extended-pages/finger-page.ui
    skin/qt/extended-pages/location-page.ui
    skin/qt/extended-pages/help-page.ui
    skin/qt/extended-pages/settings-page.ui
    skin/qt/extended-pages/telephony-page.ui
    skin/qt/extended-pages/virtual-sensors-page.ui
    skin/qt/tools.ui
    )

# moc_generate_sources
# compile specified *.ui files -> moc_ files
# RETURN_VALUE - set of generated *.h files
function(moc_generate_sources RETURN_VALUE)
    set(QT_TOP ${REPO_PREBUILTS_AEB}/qt/${BUILD_TARGET_TAG})

    foreach(UI_FILE ${ARGN})
        get_filename_component(UI_BASENAME ${UI_FILE} NAME_WE)
        # do something interesting with file 'f'
        set(MOC_IN ${CMAKE_CURRENT_LIST_DIR}/${UI_FILE})
        set(MOC_OUT moc_${UI_BASENAME}.cpp)
        add_custom_command(
            OUTPUT ${MOC_OUT}
            COMMAND ${CMAKE_CURRENT_LIST_DIR}/scripts/ld_library_run.py ${QT_TOP}/lib ${QT_TOP}/bin/moc -o ${MOC_OUT} ${MOC_IN}
            DEPENDS ${QT_TOP}/bin/moc ${MOC_IN} ${EMULATOR_UI_UIC_SRC}
            )
        set(SRC_LIST ${SRC_LIST} ${MOC_OUT})
    endforeach()
    set(${RETURN_VALUE} ${SRC_LIST} PARENT_SCOPE)
endfunction()

moc_generate_sources(
    EMULATOR_UI_MOC_SRC
    skin/qt/accelerometer-3d-widget.h
    skin/qt/angle-input-widget.h
    skin/qt/editable-slider-widget.h
    skin/qt/gl-widget.h
    skin/qt/emulator-container.h
    skin/qt/emulator-overlay.h
    skin/qt/emulator-qt-window.h
    skin/qt/emulator-qt-no-window.h
    skin/qt/event-capturer.h
    skin/qt/event-subscriber.h
    skin/qt/extended-pages/battery-page.h
    skin/qt/extended-pages/cellular-page.h
    skin/qt/extended-pages/dpad-page.h
    skin/qt/extended-pages/finger-page.h
    skin/qt/extended-pages/help-page.h
    skin/qt/extended-pages/location-page.h
    skin/qt/extended-pages/settings-page.h
    skin/qt/extended-pages/telephony-page.h
    skin/qt/extended-pages/virtual-sensors-page.h
    skin/qt/extended-window.h
    skin/qt/raised-material-button.h
    skin/qt/size-tweaker.h
    skin/qt/QtTimerImpl.h
    skin/qt/tool-window.h
    skin/qt/user-actions-counter.h
    )

# moc_generate_sources
# compile specified *.ui files -> moc_ files
# RETURN_VALUE - set of generated *.h files
function(qrc_generate_source RETURN_VALUE)
    set(QT_TOP ${REPO_PREBUILTS_AEB}/qt/${BUILD_TARGET_TAG})

    foreach(QRC_FILE ${ARGN})
        get_filename_component(QRC_BASENAME ${QRC_FILE} NAME_WE)
        # do something interesting with file 'f'
        set(RCC_IN ${CMAKE_CURRENT_LIST_DIR}/${QRC_FILE})
        set(RCC_OUT rcc_${QRC_BASENAME}.cpp)
        add_custom_command(
            OUTPUT ${RCC_OUT}
            COMMAND ${CMAKE_CURRENT_LIST_DIR}/scripts/ld_library_run.py ${QT_TOP}/lib ${QT_TOP}/bin/rcc -o ${RCC_OUT} --name ${QRC_BASENAME} ${RCC_IN}
            DEPENDS ${QT_TOP}/bin/rcc ${RCC_IN}
        )
        set(SRC_LIST ${SRC_LIST} ${RCC_OUT})
    endforeach()
    set(${RETURN_VALUE} ${SRC_LIST} PARENT_SCOPE)
endfunction()

qrc_generate_source(
    EMULATOR_UI_RCC_SRC
    skin/qt/resources.qrc
    )

add_library(emulator-ui STATIC
    ${REPO_QEMU}/images/emulator_icon.rc
    ${HW_CONFIG_DEFS_H}
    ${EMULATOR_UI_HEADERS}
    ${EMULATOR_UI_MOC_SRC}
    ${EMULATOR_UI_RCC_SRC}
    ${EMULATOR_UI_PLATFORM_SRC}
    main-common-ui.c
    skin/charmap.c
    skin/rect.c
    skin/image.c
    skin/trackball.c
    skin/keyboard.c
    skin/keycode.c
    skin/keycode-buffer.c
    skin/keyset.c
    skin/file.c
    skin/window.c
    skin/resource.c
    skin/ui.c
    skin/qt/accelerometer-3d-widget.cpp
    skin/qt/angle-input-widget.cpp
    skin/qt/editable-slider-widget.cpp
    skin/qt/emulator-container.cpp
    skin/qt/emulator-overlay.cpp
    skin/qt/emulator-qt-window.cpp
    skin/qt/emulator-qt-no-window.cpp
    skin/qt/event-capturer.cpp
    skin/qt/event-qt.cpp
    skin/qt/event-serializer.cpp
    skin/qt/event-subscriber.cpp
    skin/qt/error-dialog.cpp
    skin/qt/extended-pages/common.cpp
    skin/qt/extended-pages/battery-page.cpp
    skin/qt/extended-pages/cellular-page.cpp
    skin/qt/extended-pages/dpad-page.cpp
    skin/qt/extended-pages/finger-page.cpp
    skin/qt/extended-pages/help-page.cpp
    skin/qt/extended-pages/location-page.cpp
    skin/qt/extended-pages/settings-page.cpp
    skin/qt/extended-pages/telephony-page.cpp
    skin/qt/extended-pages/virtual-sensors-page.cpp
    skin/qt/extended-window.cpp
    skin/qt/gl-canvas.cpp
    skin/qt/gl-common.cpp
    skin/qt/gl-texture-draw.cpp
    skin/qt/gl-widget.cpp
    skin/qt/init-qt.cpp
    skin/qt/QtLooper.cpp
    skin/qt/qt-ui-commands.cpp
    skin/qt/size-tweaker.cpp
    skin/qt/stylesheet.cpp
    skin/qt/surface-qt.cpp
    skin/qt/tool-window.cpp
    skin/qt/user-actions-counter.cpp
    skin/qt/wavefront-obj-parser.cpp
    skin/qt/winsys-qt.cpp
    )

target_include_directories(emulator-ui
    PUBLIC
    # TODO(vharron): well this is ugly.  We're referencing a QEMU2 file from a library
    # shared by QEMU1/2. We should move those config files into emulator-ui
    ${REPO_QEMU2}/android-qemu2-glue/config/${BUILD_TARGET_TAG}
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${REPO_PREBUILTS_AEB}/qt/common/include/QtCore
    ${REPO_PREBUILTS_AEB}/qt/common/include/QtGui
    ${REPO_PREBUILTS_AEB}/qt/common/include/QtWidgets
    ${REPO_PREBUILTS_AEB}/qt/common/include
    )

set(QT_BIN_DIR ${REPO_PREBUILTS_AEB}/qt/${BUILD_TARGET_TAG}/bin)
set(QT_LIB_DIR ${REPO_PREBUILTS_AEB}/qt/${BUILD_TARGET_TAG}/lib)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(EMULATOR_UI_PLATFORM_LINK
        -lpulse
        -lX11
        )
    file(GLOB QT_PREBUILT_LIBRARY_PATHS ${QT_LIB_DIR}/*.so*)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    file(GLOB QT_PREBUILT_LIBRARY_PATHS ${QT_LIB_DIR}/*.dylib)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(EMULATOR_UI_PLATFORM_LINK
        -L${QT_BIN_DIR}
        -lmingw32
        -lqtmain
        -lvfw32
        )
    file(GLOB QT_PREBUILT_LIBRARY_PATHS ${QT_BIN_DIR}/*.dll)
endif()

file(COPY ${QT_PREBUILT_LIBRARY_PATHS} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
file(GLOB QT_PLUGINS ${REPO_PREBUILTS_AEB}/qt/${BUILD_TARGET_TAG}/plugins/*)
file(COPY ${QT_PLUGINS} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

target_link_libraries(emulator-ui
    OpenGLESDispatch
    -L${QT_LIB_DIR}
    -lQt5Widgets
    -lQt5Gui
    -lQt5Core
    ${EMULATOR_UI_PLATFORM_LINK}
    -lpng
    )

target_compile_options(
    emulator-ui
    PRIVATE
    -DUSE_MMX=1
    -mmmx
    )

################################################################################
# emulator_ui_unittests

add_executable(emulator_ui_unittests
    skin/keycode_unittest.cpp
    skin/keycode-buffer_unittest.cpp
    skin/rect_unittest.cpp
    )

target_link_libraries(emulator_ui_unittests
    emulator-ui
    android-emu
    gtest
    )

target_compile_options(emulator_ui_unittests
    PRIVATE
    -O0
    )

################################################################################
# emulator
# This is the launcher binary that starts the emualtor-${ARCH} executable that
# matches the guest architecture.

add_executable(emulator
    main-emulator.cpp
    ${REPO_QEMU}/images/emulator_icon.rc
    )

target_compile_options(emulator
    PUBLIC
    -DCONFIG_QT
    )

target_link_libraries(emulator
    PRIVATE
    android-emu
    )

################################################################################
# emulator-check: a standalone question answering service

add_executable(emulator-check
    emulator-check/main-emulator-check.cpp
    emulator-check/PlatformInfo.cpp
    ${REPO_QEMU}/images/emulator_icon.rc
    )

target_link_libraries(emulator-check
    android-emu
    )

