#include "android/emulation/control/GrpcServices.h"

#include "android/emulation/control/EmulatorService.h"  // for EmulatorContr...

namespace android {
namespace emulation {
namespace control {
class RtcBridge;
}  // namespace control
}  // namespace emulation
}  // namespace android

using android::emulation::control::GrpcServices;
using android::emulation::control::EmulatorControllerService;

std::unique_ptr<EmulatorControllerService> GrpcServices::g_controler_service = nullptr;

void GrpcServices::setup(int port,
                    const AndroidConsoleAgents* const consoleAgents,
                    RtcBridge* bridge) {
    g_controler_service = EmulatorControllerService::Builder()
                                  .withConsoleAgents(consoleAgents)
                                  .withPort(port)
                                  .withRtcBridge(bridge)
                                  .build();
}

void GrpcServices::teardown() {
    if (g_controler_service) {
        g_controler_service->stop();
    }
}
