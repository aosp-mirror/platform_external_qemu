#include "android/emulation/control/GrpcServices.h"
using android::emulation::control::GrpcServices;
using android::emulation::control::EmulatorControllerService;

std::unique_ptr<EmulatorControllerService> GrpcServices::g_controler_service = nullptr;

void GrpcServices::setup(std::string args,
                    const AndroidConsoleAgents* const consoleAgents) {
    int grpc = 0;
    if (sscanf(args.c_str(), "%d", &grpc) == 1) {
        g_controler_service = EmulatorControllerService::Builder()
                                      .withConsoleAgents(consoleAgents)
                                      .withPort(grpc)
                                      .build();
    }
}

void GrpcServices::teardown() {
    if (g_controler_service) {
        g_controler_service->stop();
    }
}