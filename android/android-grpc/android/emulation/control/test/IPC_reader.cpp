// Copyright (C) 2020 The Android Open Source Project
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

// Remote test runner that will obtain bytes from the calling process
// and report back the crc32.

#ifdef _MSC_VER
#include "msvc-posix.h"
#include "msvc-getopt.h"
#else
#include <getopt.h>                                          // for getopt_long
#endif

#include <grpcpp/grpcpp.h>                                   // for CreateCu...
#include <stdlib.h>                                          // for atoi
#include <zlib.h>                                            // for crc32
#include <cstdint>                                           // for uint8_t
#include <ctime>                                             // for clock
#include <memory>                                            // for unique_ptr
#include <string>                                            // for string
#include <vector>                                            // for vector

#include "android/base/Log.h"                                // for LogStrea...
#include "android/base/StringView.h"                         // for StringView
#include "android/base/memory/SharedMemory.h"                // for SharedMe...
#include "android/base/sockets/ScopedSocket.h"               // for ScopedSo...
#include "android/base/sockets/SocketUtils.h"                // for socketRe...
#include "android/base/system/System.h"                      // for System
#include "android/emulation/control/GrpcServices.h"          // for Emulator...
#include "android/emulation/control/test/TestEchoService.h"  // for ServerCo...
#include "google/protobuf/empty.pb.h"                        // for Empty
#include "grpcpp/security/credentials.h"                     // for Insecure...
#include "grpcpp/support/channel_arguments.h"                // for ChannelA...
#include "ipc_test_service.grpc.pb.h"                        // for TestRunner
#include "ipc_test_service.pb.h"                             // for Test
#include "test_echo_service.grpc.pb.h"                       // for TestEcho
#include "test_echo_service.pb.h"                            // for Msg

using namespace android::base;
using namespace android::emulation::control;

namespace android {
namespace emulation {
namespace control {

using grpc::ServerContext;
using grpc::Status;

class TestRunnerImpl final : public TestRunner::Service {
public:
    Status runTest(ServerContext* context,
                   const Test* request,
                   Test* response) override {
        // Note: Might not be perfect way of measuring cpu time.
        std::clock_t c_start = std::clock();
        switch (request->target()) {
            case Test::Nothing:
                break;
            case Test::SharedMemory:
                response->set_chksum(from_shared_mem(
                        request->handle(), request->size(),
                        request->mode() == Test::Shared
                                ? SharedMemory::ShareType::SHARED_MEMORY
                                : SharedMemory::ShareType::FILE_BACKED));
                break;
            case Test::Grpc:
                response->set_chksum(from_grpc(
                        "localhost:" + std::to_string(request->port())));
                break;
            case Test::RawSocket:
                response->set_chksum(
                        from_socket(request->port(), request->size()));
                break;
        }
        std::clock_t c_end = std::clock();
        response->set_cputime(c_end - c_start);

        return Status::OK;
    }

private:
    // Calculate a simple hash of the memory region.
    // This forces a memory read on the region.
    uint64_t read_region(const uint8_t* src, size_t size) {
            return crc32(0, src, size);
    }

    // Calculate a simple hash from the shared memory region
    // in the given mode.
    uint64_t from_shared_mem(std::string handle,
                             size_t size,
                             SharedMemory::ShareType backend) {
        SharedMemory mReader(handle, size, backend);
        mReader.open(SharedMemory::AccessMode::READ_ONLY);
        return read_region((uint8_t*)*mReader, size);
    }

    // Calculate the hash by receiving all bytes over a socket
    // and calculating the hash.
    uint64_t from_socket(int port, size_t size) {
        std::vector<uint8_t> data(size);
        ScopedSocket s(socketTcp4LoopbackClient(port));
        if (!s.valid()) {
            LOG(ERROR) << "Unable to establish connection.";
            return -1;
        }
        if (!socketRecvAll(s.get(), data.data(), data.size())) {
            LOG(ERROR) << "Did not receive all bytes";
            return -1;
        }
        return read_region(data.data(), data.size());
    }

    uint64_t from_grpc(std::string address) {
        grpc::ChannelArguments ch_args;
        ch_args.SetMaxReceiveMessageSize(-1);
        auto channel = grpc::CreateCustomChannel(
                address, ::grpc::InsecureChannelCredentials(), ch_args);
        auto client = TestEcho::NewStub(channel);
        grpc::ClientContext ctx;

        // You might see unexpected failures with fail fast semantics
        // and transient connection issues.
        ctx.set_wait_for_ready(true);
        Msg response;
        auto status =
                client->data(&ctx, ::google::protobuf::Empty(), &response);
        return read_region((uint8_t*)response.data().data(),
                           response.data().size());
    }
};
}  // namespace control
}  // namespace emulation
}  // namespace android

static int port = -1;

static struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                       {0, 0, 0, 0}};

static void parseArgs(int argc, char** argv) {
    int long_index = 0;
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) !=
           -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            default:
                break;
        }
    }
}

int main(int argc, char** argv) {
    parseArgs(argc, argv);
    EmulatorControllerService::Builder builder;
    auto ecs = builder.withService(new TestRunnerImpl())
                       .withPortRange(port, port + 1)
                       .withLogging(true)
                       .build();
    while (true) {
        System::get()->sleepMs(1000);
    };
    return 0;
}
