// Copyright (C) 2018 The Android Open Source Project
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

#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>

#include "android/console.h"
#include "android/emulation/control/EmulatorService.h"
#include "android/emulation/control/emulator_controller.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace android {
namespace emulation {
namespace control {

class EmulatorControllerServiceImpl : public EmulatorControllerService {
public:
    void stop() override { mServer->Shutdown(); }

    EmulatorControllerServiceImpl(EmulatorController::Service* service,
                                  grpc::Server* server)
        : mService(service), mServer(server) {}

private:
    std::unique_ptr<EmulatorController::Service> mService;
    std::unique_ptr<grpc::Server> mServer;
};

// Logic and data behind the server's behavior.
class EmulatorControllerImpl final : public EmulatorController::Service {
public:
    EmulatorControllerImpl(const AndroidConsoleAgents* agents)
        : mAgents(agents) {}

    Status setRotation(ServerContext* context,
                       const Rotation* request,
                       Rotation* reply) override {
        mAgents->emu->rotate((SkinRotation)request->rotation());
        reply->set_rotation((Rotation_SkinRotation)mAgents->emu->getRotation());
        return Status::OK;
    }

    //TODO(jansene): Hook up the other agents.
private:
    const AndroidConsoleAgents* mAgents;
};

using Builder = EmulatorControllerService::Builder;

Builder::Builder() : mCredentials{grpc::InsecureServerCredentials()} {}

Builder& Builder::withConsoleAgents(
        const AndroidConsoleAgents* const consoleAgents) {
    mAgents = consoleAgents;
    return *this;
}
Builder& Builder::withCredentials(
        std::shared_ptr<grpc::ServerCredentials> credentials) {
    mCredentials = credentials;
    return *this;
}

Builder& Builder::withPort(int port) {
    mPort = port;
    return *this;
}

std::unique_ptr<EmulatorControllerService> Builder::build() {
    if (mAgents == nullptr) {
        // Excuse me?
        return nullptr;
    }

    std::string server_address = "0.0.0.0:" + std::to_string(mPort);
    std::unique_ptr<EmulatorController::Service> controller(
            new EmulatorControllerImpl(mAgents));

    ServerBuilder builder;
    builder.AddListeningPort(server_address, mCredentials);
    builder.RegisterService(controller.release());
    auto service = builder.BuildAndStart();
    if (!service)
        return nullptr;

    return std::unique_ptr<EmulatorControllerService>(
            new EmulatorControllerServiceImpl(controller.release(),
                                              service.release()));
}

}  // namespace control
}  // namespace emulation
}  // namespace android

/* void RunServer() { */
/*     std::string server_address("0.0.0.0:50051"); */
/*     //    android::emulation::control::EmulatorControllerImpl service; */

/*     ServerBuilder builder; */
/*     // Listen on the given address without any authentication mechanism. */
/*     builder.AddListeningPort(server_address,
 * grpc::InsecureServerCredentials()); */
/*     // Register "service" as the instance through which we'll communicate
 * with */
/*     // clients. In this case it corresponds to an *synchronous* service. */
/*     builder.RegisterService(&service); */
/*     // Finally assemble the server. */
/*     std::unique_ptr<Server> server(builder.BuildAndStart()); */
/*     std::cout << "Server listening on " << server_address << std::endl; */

/*     // Wait for the server to shutdown. Note that some other thread must be
 */
/*     // responsible for shutting down the server for this call to ever return.
 */
/*     server->Wait(); */
/* } */
