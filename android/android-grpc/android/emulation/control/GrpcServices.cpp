#include "android/emulation/control/GrpcServices.h"

#include "android/base/files/PathUtils.h"               // for PathUtils
#include "android/emulation/ConfigDirs.h"               // for ConfigDirs
#include "android/emulation/control/EmulatorService.h"  // for EmulatorContr...
#include "android/emulation/control/RtcBridge.h"        // for RtcBridge


using android::emulation::control::EmulatorControllerService;
using android::emulation::control::GrpcServices;
using android::emulation::control::NopRtcBridge;
using android::emulation::control::RtcBridge;

const std::string GrpcServices::kCertFileName{"emulator-grpc.cer"};
const std::string GrpcServices::kPrivateKeyFileName{"emulator-grpc.key"};

std::unique_ptr<EmulatorControllerService> GrpcServices::g_controler_service =
        nullptr;
std::unique_ptr<RtcBridge> GrpcServices::g_rtc_bridge = nullptr;

int GrpcServices::setup(int port,
                        const AndroidConsoleAgents* const consoleAgents,
                        RtcBridge* rtcBridge,
                        const char* waterfall) {
    // Return the active port if we are already running.
    if (g_controler_service) {
        return g_controler_service->port();
    }

    g_rtc_bridge.reset(rtcBridge);
    g_rtc_bridge->start();

    auto builder =
            EmulatorControllerService::Builder()
                    .withConsoleAgents(consoleAgents)
                    .withCertAndKey(
                            base::PathUtils::join(
                                    android::ConfigDirs::getUserDirectory(),
                                    kCertFileName),
                            base::PathUtils::join(
                                    android::ConfigDirs::getUserDirectory(),
                                    kPrivateKeyFileName))
                    .withPort(port)
                    .withWaterfall(waterfall)
                    .withRtcBridge(g_rtc_bridge.get());

                            g_controler_service = builder.build();

    return (g_controler_service) ? builder.port() : -1;
}

void GrpcServices::teardown() {
    if (g_controler_service) {
        g_controler_service->stop();
    }
}
