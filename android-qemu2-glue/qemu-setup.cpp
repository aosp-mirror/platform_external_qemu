// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifdef _MSC_VER
extern "C" {
#include "qemu/osdep.h"
}
#endif

#include "aemu/base/logging/LogSeverity.h"
#include "android-qemu2-glue/qemu-setup.h"
#include "android/base/system/System.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>
#include <ostream>
#include <random>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "aemu/base/CpuUsage.h"
#include "aemu/base/Log.h"
#include "aemu/base/Tracing.h"
#include "aemu/base/Uuid.h"
#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/files/IniFile.h"
#include "aemu/base/files/MemStream.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/ScopedPtr.h"
#include "aemu/base/process/Process.h"
#include "android-qemu2-glue/android_qemud.h"
#include "android-qemu2-glue/audio-capturer.h"
#include "android-qemu2-glue/audio-output.h"
#include "android-qemu2-glue/base/async/CpuLooper.h"
#include "android-qemu2-glue/base/async/Looper.h"
#include "android-qemu2-glue/emulation/DmaMap.h"
#include "android-qemu2-glue/emulation/VmLock.h"
#include "android-qemu2-glue/emulation/android_address_space_device.h"
#include "android-qemu2-glue/emulation/android_pipe_device.h"
#include "android-qemu2-glue/emulation/charpipe.h"
#include "android-qemu2-glue/emulation/goldfish_sync.h"
#include "android-qemu2-glue/emulation/virtio-wifi.h"
#include "android-qemu2-glue/emulation/virtio_vsock_transport.h"
#include "android-qemu2-glue/looper-qemu.h"
#include "android-qemu2-glue/net-android.h"
#include "android-qemu2-glue/netsim/netsim.h"
#include "android-qemu2-glue/proxy/slirp_proxy.h"
#include "android-qemu2-glue/snapshot_compression.h"
#include "android/android.h"
#include "android/avd/info.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/HangDetector.h"
#include "android/crashreport/detectors/CrashDetectors.h"
#include "android/emulation/AudioCaptureEngine.h"
#include "android/emulation/AudioOutputEngine.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/QemuMiscPipe.h"
#include "android/emulation/bluetooth/EmulatedBluetoothService.h"
#include "android/emulation/control/EmulatorAdvertisement.h"
#include "android/emulation/control/EmulatorService.h"
#include "android/emulation/control/GrpcServices.h"
#include "android/emulation/control/UiController.h"
#include "android/emulation/control/adb/AdbService.h"
#include "android/emulation/control/incubating/CarService.h"
#include "android/emulation/control/incubating/ModemService.h"
#include "android/emulation/control/incubating/SensorService.h"
#include "android/emulation/control/snapshot/SnapshotService.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/waterfall/WaterfallService.h"
#include "android/emulation/stats/EmulatorStats.h"
#include "android/emulation/virtio_vsock_device.h"
#include "android/gpu_frame.h"
#include "android/skin/winsys.h"
#include "snapshot/interface.h"
#include "android/utils/Random.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "android/version.h"
#include "host-common/DmaMap.h"
#include "host-common/VmLock.h"
#include "host-common/address_space_device.h"
#include "host-common/address_space_device.hpp"
#include "host-common/crash-handler.h"
#include "host-common/feature_control.h"
#include "host-common/record_screen_agent.h"
#include "snapshot_service.grpc.pb.h"

#ifdef ANDROID_WEBRTC
#include "android/emulation/control/RtcService.h"
#include "android/emulation/control/RtcServiceV2.h"
#endif

extern "C" {

#include "android/proxy/proxy_int.h"
#include "qemu/abort.h"
#include "qemu/main-loop.h"
#include "qemu/osdep.h"
#include "qemu/thread.h"
#include "sysemu/device_tree.h"
#include "sysemu/sysemu.h"

extern char* android_op_ports;

// TODO: Remove op_http_proxy global variable.
extern char* op_http_proxy;

}  // extern "C"

using android::DmaMap;
using android::VmLock;
using android::base::PathUtils;
using android::emulation::control::EmulatorAdvertisement;
using android::emulation::control::EmulatorControllerService;
using android::emulation::control::EmulatorProperties;

std::string get_display_name();

// Tansfrom a const char* to std::string returning empty "" for nullptr
inline static std::string to_string(const char* str) {
    return str ? std::string(str) : "";
}

extern "C" void ranchu_device_tree_setup(void* fdt) {
    /* fstab */
    qemu_fdt_add_subnode(fdt, "/firmware/android/fstab");
    qemu_fdt_setprop_string(fdt, "/firmware/android/fstab", "compatible",
                            "android,fstab");

    char* system_path =
            feature_is_enabled(kFeature_SystemAsRoot)
                    ? nullptr
                    : avdInfo_getSystemImageDevicePathInGuest(
                              getConsoleAgents()->settings->avdInfo());
    if (system_path) {
        qemu_fdt_add_subnode(fdt, "/firmware/android/fstab/system");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system",
                                "compatible", "android,system");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "dev",
                                system_path);
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system",
                                "fsmgr_flags", "wait");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system",
                                "mnt_flags", "ro");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "type",
                                "ext4");
        free(system_path);
    }

    char* vendor_path = avdInfo_getVendorImageDevicePathInGuest(
            getConsoleAgents()->settings->avdInfo());
    if (vendor_path) {
        qemu_fdt_add_subnode(fdt, "/firmware/android/fstab/vendor");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor",
                                "compatible", "android,vendor");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "dev",
                                vendor_path);
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor",
                                "fsmgr_flags", "wait");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor",
                                "mnt_flags", "ro");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "type",
                                "ext4");
        free(vendor_path);
    }
}

extern "C" void rng_random_generic_read_random_bytes(void* buf, int size) {
    if (size <= 0)
        return;

    android::base::MemStream stream;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> uniform_dist(0, 255);  // fill a byte

    for (int i = 0; i < size; ++i) {
        stream.putByte(uniform_dist(gen));
    }
    stream.read(buf, size);
}

bool qemu_android_emulation_early_setup() {
    // Ensure that the looper is set for the main thread and for any
    // future thread created by QEMU.
    qemu_looper_setForThread();
    qemu_thread_register_setup_callback(qemu_looper_setForThread);

    qemu_abort_set_handler((QemuAbortHandler)&crashhandler_die_format_v);
    qemu_crash_dump_message_func_set(
            (QemuCrashDumpMessageFunc)&crashhandler_append_message_format_v);

    // Make sure we override the ctrl-C handler as soon as possible.
    qemu_set_ctrlc_handler(&skin_winsys_quit_request);

    // Ensure charpipes i/o are handled properly.
    main_loop_register_poll_callback(qemu_charpipe_poll);

    // Setup qemud and register qemud-related snapshot callbacks.
    android_qemu2_qemud_init();

    // Ensure the VmLock implementation is setup.
    VmLock* vmLock = new qemu2::VmLock();
    VmLock* prevVmLock = VmLock::set(vmLock);
    CHECK(prevVmLock == nullptr) << "Another VmLock was already installed!";

    // Ensure the DmaMap implementation is setup.
    DmaMap* dmaMap = new qemu2::DmaMap();
    DmaMap* prevDmaMap = DmaMap::set(dmaMap);
    CHECK(prevDmaMap == nullptr) << "Another DmaMap was already installed!";

    // Initialize host pipe service.
    if (!qemu_android_pipe_init(vmLock)) {
        return false;
    }

    // Initialize host sync service.
    if (!qemu_android_sync_init(vmLock)) {
        return false;
    }

    virtio_vsock_device_set_ops(virtio_vsock_device_get_host_ops());

    auto vm = getConsoleAgents()->vm;
    android::emulation::goldfish_address_space_set_vm_operations(vm);

    qemu_set_address_space_device_control_ops(
            (struct qemu_address_space_device_control_ops*)
                    get_address_space_device_control_ops());

    qemu_android_address_space_device_init();

    androidSnapshot_initialize(vm, getConsoleAgents()->emu);
    qemu_snapshot_compression_setup();

    android::emulation::AudioCaptureEngine::set(
            new android::qemu::QemuAudioCaptureEngine());

    android::emulation::AudioCaptureEngine::set(
            new android::qemu::QemuAudioInputEngine(),
            android::emulation::AudioCaptureEngine::AudioMode::AUDIO_INPUT);

    android::emulation::AudioOutputEngine::set(
            new android::qemu::QemuAudioOutputEngine());

    auto opts = getConsoleAgents()->settings->android_cmdLineOptions();
    std::string name = get_display_name();
    register_netsim(to_string(opts->packet_streamer_endpoint),
                    to_string(opts->rootcanal_controller_properties_file),
                    name);
    return true;
}

static std::unique_ptr<EmulatorAdvertisement> advertiser;
static std::unique_ptr<EmulatorControllerService> grpcService;

bool qemu_android_ports_setup() {
    return android_ports_setup(getConsoleAgents(), true);
}

static const std::string kCertFileName{"emulator-grpc.cer"};
static const std::string kPrivateKeyFileName{"emulator-grpc.key"};

// Generates a secure base64 encoded token of
// |cnt| bytes.
static std::string generateToken(int cnt) {
    char buf[cnt];
    if (!android::generateRandomBytes(buf, sizeof(buf))) {
        return "";
    }

    const size_t kBase64Len = 4 * ((cnt + 2) / 3);
    char base64[kBase64Len];
    int len = proxy_base64_encode(buf, cnt, base64, kBase64Len);
    // len < 0 can only happen if we calculate kBase64Len incorrectly..
    assert(len > 0);
    return std::string(base64, len);
}

int qemu_grpc_port() {
    if (grpcService) {
        return grpcService->port();
    }

    return -1;
}

std::string get_display_name() {
    std::string displayName;
    // Ideally we use the display name, which is not available for all
    // systems (fuchsia).
    auto avdInfo = getConsoleAgents()->settings->avdInfo();
    auto iniFile = avdInfo ? avdInfo_getConfigIni(avdInfo) : nullptr;
    if (iniFile) {
        auto cfgIni = reinterpret_cast<const android::base::IniFile*>(iniFile);
        displayName =
                cfgIni->getString("avd.ini.displayname",
                                  getConsoleAgents()->settings->hw()->avd_name);
    } else {
        displayName = avdInfo_getName(getConsoleAgents()->settings->avdInfo());
    }
    return displayName;
}

int qemu_setup_grpc() {
    if (grpcService)
        return grpcService->port();

    std::string displayName = get_display_name();

    auto avdInfo = getConsoleAgents()->settings->avdInfo();
    auto contentPath = avdInfo_getContentPath(avdInfo);
    EmulatorProperties props{
            {"port.serial", std::to_string(android_serial_number_port)},
            {"emulator.build", EMULATOR_BUILD_STRING},
            {"emulator.version", EMULATOR_VERSION_STRING},
            {"port.adb", std::to_string(android_adb_port)},
            {"avd.name", displayName},
            {"avd.id", avdInfo_getId(getConsoleAgents()->settings->avdInfo())},
            {"avd.dir", contentPath ? contentPath : ""},
            {"cmdline", getConsoleAgents()->settings->android_cmdLine()}};

    int grpc_start = android_serial_number_port + 3000;
    int grpc_end = grpc_start + 1000;
    bool has_grpc_flag =
            getConsoleAgents()->settings->android_cmdLineOptions()->grpc;
    std::string address = "[::1]";

    if (has_grpc_flag &&
        sscanf(getConsoleAgents()->settings->android_cmdLineOptions()->grpc,
               "%d", &grpc_start) == 1) {
        grpc_end = grpc_start + 1;
        address = "[::]";
    }

    auto emulator = android::emulation::control::getEmulatorController(
            getConsoleAgents());
    auto h2o = android::emulation::control::getWaterfallService(
            getConsoleAgents()->settings->android_cmdLineOptions()->waterfall);
    auto snapshot = android::emulation::control::getSnapshotService();
    auto uiController = android::emulation::control::getUiControllerService(
            getConsoleAgents());
    auto adb = android::emulation::control::getAdbService();
    auto stats = android::emulation::stats::getStatsService();
    auto modem = android::emulation::control::incubating::getModemService(
            getConsoleAgents());
    auto car = android::emulation::control::incubating::getCarService(
            getConsoleAgents());
    auto sensor = android::emulation::control::incubating::getSensorService(
            getConsoleAgents());
    auto bluetooth =
            android::emulation::bluetooth::getEmulatedBluetoothService();
    auto builder =
            EmulatorControllerService::Builder()
                    .withConsoleAgents(getConsoleAgents())
                    .withLogging(VERBOSE_CHECK(grpc))
                    .withPortRange(grpc_start, grpc_end)
                    .withCertAndKey(getConsoleAgents()
                                            ->settings->android_cmdLineOptions()
                                            ->grpc_tls_cer,
                                    getConsoleAgents()
                                            ->settings->android_cmdLineOptions()
                                            ->grpc_tls_key,
                                    getConsoleAgents()
                                            ->settings->android_cmdLineOptions()
                                            ->grpc_tls_ca)
                    .withVerboseLogging(android_verbose)
                    .withAllowList(getConsoleAgents()
                                           ->settings->android_cmdLineOptions()
                                           ->grpc_allowlist)
                    .withAddress(address)
                    .withService(modem)
                    .withService(emulator)
                    .withService(h2o)
                    .withService(snapshot)
                    .withService(uiController)
                    .withService(stats)
                    .withService(car)
                    .withService(sensor)
                    .withSecureService(adb)
                    .withService(bluetooth);

    int timeout = 0;
    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->idle_grpc_timeout &&
        sscanf(getConsoleAgents()
                       ->settings->android_cmdLineOptions()
                       ->idle_grpc_timeout,
               "%d", &timeout) == 1) {
        builder.withIdleTimeout(std::chrono::seconds(timeout));
    }
    bool useToken = !has_grpc_flag || getConsoleAgents()
                            ->settings->android_cmdLineOptions()
                            ->grpc_use_token;

    if (useToken) {
        const int of64Bytes = 64;
        auto token = generateToken(of64Bytes);
        builder.withAuthToken(token);
        props["grpc.token"] = token;
    }
    if (!has_grpc_flag ||
        getConsoleAgents()->settings->android_cmdLineOptions()->grpc_use_jwt) {
        auto jwkDir = android::base::pj(
                {android::ConfigDirs::getDiscoveryDirectory(),
                 std::to_string(
                         android::base::System::get()->getCurrentProcessId()),
                 "jwks", android::base::Uuid::generate().toString()});
        auto jwkLoadedFile = android::base::pj(jwkDir, "active.jwk");
        if (path_mkdir_if_needed(jwkDir.c_str(), 0700) != 0) {
            dfatal("Failed to create jwk directory %s", jwkDir.c_str());
        }
        props["grpc.jwks"] = jwkDir;
        props["grpc.jwk_active"] = jwkLoadedFile;
        builder.withJwtAuthDiscoveryDir(jwkDir, jwkLoadedFile);
        if (!getConsoleAgents()
                     ->settings->android_cmdLineOptions()
                     ->grpc_use_token) {
            dwarning(
                    "The emulator now requires a signed jwt token for gRPC "
                    "access! Use the -grpc flag if you really want an open "
                    "unprotected grpc port");
        }
    }

#ifdef ANDROID_WEBRTC
    // There are two versions of rtc service and we keep both of them to
    // maintain backward compatibility.
    auto rtcSvc = android::emulation::control::getRtcService(
            getConsoleAgents()->settings->android_cmdLineOptions()->turncfg,
            getConsoleAgents()->settings->android_cmdLineOptions()->dump_audio,
            true);
    builder.withService(rtcSvc);

    auto rtcSvcV2 = android::emulation::control::v2::getRtcService(
            getConsoleAgents()->settings->android_cmdLineOptions()->turncfg,
            getConsoleAgents()->settings->android_cmdLineOptions()->dump_audio,
            true);
    builder.withService(rtcSvcV2);

#endif
    int port = -1;
    grpcService = builder.build();

    if (grpcService) {
        port = grpcService->port();

        props["grpc.port"] = std::to_string(port);
        props["grpc.allowlist"] = builder.allowlist();
        if (getConsoleAgents()
                    ->settings->android_cmdLineOptions()
                    ->grpc_tls_cer) {
            props["grpc.server_cert"] =
                    getConsoleAgents()
                            ->settings->android_cmdLineOptions()
                            ->grpc_tls_cer;
        }
        if (getConsoleAgents()
                    ->settings->android_cmdLineOptions()
                    ->grpc_tls_ca) {
            props["grpc.ca_root"] = getConsoleAgents()
                                            ->settings->android_cmdLineOptions()
                                            ->grpc_tls_ca;
        }
    }

    bool userWantsGrpc =
            getConsoleAgents()->settings->android_cmdLineOptions()->grpc ||
            getConsoleAgents()
                    ->settings->android_cmdLineOptions()
                    ->grpc_tls_ca ||
            getConsoleAgents()
                    ->settings->android_cmdLineOptions()
                    ->grpc_tls_key ||
            getConsoleAgents()
                    ->settings->android_cmdLineOptions()
                    ->grpc_tls_ca ||
            getConsoleAgents()
                    ->settings->android_cmdLineOptions()
                    ->grpc_use_token;
    if (!grpcService && userWantsGrpc) {
        derror("Failed to start grpc service, even though it was explicitly "
               "requested.");
        exit(1);
    }

    advertiser = std::make_unique<EmulatorAdvertisement>(std::move(props));
    advertiser->garbageCollect();
    advertiser->write();

    return port;
}

bool qemu_android_emulation_setup() {
    android::base::initializeTracing();
    if (std::getenv("VPERFETTO_TRACE_ENABLED")) {
        android::base::enableTracing();
    }

    android_qemu_init_slirp_shapers();

    if (!qemu_android_setup_http_proxy(op_http_proxy)) {
        return false;
    }

    android::emulation::virtio_vsock_new_transport_init();

    if (!android_emulation_setup(getConsoleAgents(), true)) {
        return false;
    }

    bool isRunningFuchsia = !getConsoleAgents()->settings->android_qemu_mode();
    if (isRunningFuchsia) {
        // For fuchsia we only enable thr gRPC port if it is explicitly
        // requested.
        int grpc;
        if (getConsoleAgents()->settings->android_cmdLineOptions()->grpc &&
            sscanf(getConsoleAgents()->settings->android_cmdLineOptions()->grpc,
                   "%d", &grpc) == 1) {
            qemu_setup_grpc();
        }
    } else {
        // If you explicitly request adb & telnet port, you must explicitly
        // set the grpc port.
        if (android_op_ports == nullptr ||
            getConsoleAgents()->settings->android_cmdLineOptions()->grpc) {
            qemu_setup_grpc();
        }
    }

    // Make sure we have the proper loopers available for frame sharing and
    // video recording
    gpu_initialize_recorders();

    // We are sharing video, time to launch the shared memory recorder.
    // Note, the webrtc module could have started the shared memory module
    // with a differrent fps suggestion. (Fps is mainly used by clients as a
    // suggestion on how often to check for new frames)
    if (getConsoleAgents()->settings->android_cmdLineOptions()->share_vid) {
        if (!getConsoleAgents()->record->startSharedMemoryModule(60)) {
            dwarning(
                    "Unable to setup a shared memory handler, last errno: "
                    "%d",
                    errno);
        }
    }

    if (!getConsoleAgents()->settings->android_qemu_mode())
        return true;

    android::base::ScopedCPtr<const char> arch(
            avdInfo_getTargetCpuArch(getConsoleAgents()->settings->avdInfo()));
    const int nCores = getConsoleAgents()->settings->hw()->hw_cpu_ncore;
    for (int i = 0; i < nCores; ++i) {
        auto cpuLooper = new android::qemu::CpuLooper(i);
        android::crashreport::CrashReporter::get()
                ->hangDetector()
                .addWatchedLooper(cpuLooper);
        android::base::CpuUsage::get()->addLooper(
                (int)android::base::CpuUsage::UsageArea::Vcpu + i, cpuLooper);
    }

    if (getConsoleAgents()->settings->has_cmdLineOptions() &&
        getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->detect_image_hang) {
        dwarning("Enabled event overflow detection.\n");
        const int kMaxEventsDropped = 1024;
        android::crashreport::CrashReporter::get()
                ->hangDetector()
                .addPredicateCheck(
                        new android::crashreport::TimedHangDetector(
                                60 * 1000,
                                new android::crashreport::HeartBeatDetector(
                                        get_guest_heart_beat_count)),
                        "The guest is not sending heartbeat update, "
                        "probably "
                        "stalled.")
                .addPredicateCheck(
                        [] {
                            return getConsoleAgents()
                                           ->user_event->eventsDropped() >
                                   kMaxEventsDropped;
                        },
                        "The goldfish event queue is overflowing, the "
                        "system "
                        "is no longer responding.");
    }
    if (feature_is_enabled(kFeature_VirtioWifi)) {
        virtio_wifi_set_mac_prefix(android_serial_number_port);
    }

    return true;
}

void qemu_android_emulation_teardown() {
    android::qemu::skipTimerOps();
    androidSnapshot_finalize();
    android_emulation_teardown();
    if (grpcService) {
        // Explicitly cleanup resources. We do not want to do this at program
        // exit as we may be holding on to loopers, which threads have likely
        // been destroyed at that point.
        grpcService->stop();
        grpcService = nullptr;
    }

    // Let's manually disconnect netsim so we can collect metrics etc..
    close_netsim();

    android::base::disableTracing();
    if (advertiser)
        advertiser->remove();
}
