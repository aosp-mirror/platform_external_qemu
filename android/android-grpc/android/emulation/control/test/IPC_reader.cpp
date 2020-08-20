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
// clang-format off
#include "msvc-posix.h"
#include "msvc-getopt.h"
// clang-format on
#else
#include <getopt.h>  // for getopt_long
#endif

#include <errno.h>          // for errno
#include <grpcpp/grpcpp.h>  // for Status
#include <stdlib.h>         // for atoi
#include <zlib.h>           // for crc32
#include <cstdint>          // for uint8_t
#include <ctime>            // for clock
#include <functional>       // for __bind
#include <memory>           // for unique_ptr
#include <ostream>          // for ostream
#include <string>           // for string
#include <thread>           // for thread
#include <vector>           // for vector

#include "android/base/Log.h"                                // for LogStrea...
#include "android/base/async/AsyncSocketServer.h"            // for AsyncSoc...
#include "android/base/memory/SharedMemory.h"                // for SharedMe...
#include "android/base/sockets/ScopedSocket.h"               // for ScopedSo...
#include "android/base/sockets/SocketUtils.h"                // for socketSe...
#include "android/base/system/System.h"                      // for System
#include "android/base/testing/TestLooper.h"                 // for TestLooper
#include "android/emulation/control/GrpcServices.h"          // for Emulator...
#include "android/emulation/control/test/TestEchoService.h"  // for TestEcho...
#include "ipc_test_service.grpc.pb.h"                        // for TestRunner
#include "ipc_test_service.pb.h"                             // for Test

using namespace android::base;
using namespace android::emulation::control;

namespace android {
namespace emulation {
namespace control {

using grpc::ServerContext;
using grpc::Status;

// A socket server that will just write all the data to the socket
// that connects to it.
class SimpleReplySocket {
public:
    SimpleReplySocket(std::vector<uint8_t> data) : mData(data) {
        mRunner = std::make_unique<std::thread>([this]() { loop(); });
    }

    ~SimpleReplySocket() {
        mDone = true;
        mRunner->join();
    }

    bool startServer() {
        auto callback = std::bind(&SimpleReplySocket::onConnection, this,
                                  std::placeholders::_1);
        mServer = AsyncSocketServer::createTcpLoopbackServer(
                0, callback, AsyncSocketServer::LoopbackMode::kIPv4, &mLooper);
        if (!mServer.get()) {
            return false;
        }
        mPort = mServer->port();
        mServer->startListening();
        return true;
    }

    // On every connection, increment the counter, read one byte from the
    // socket, then close it! Note that this doesn't start listening for
    // new connections, this is left to the test itself.
    bool onConnection(int socket) {
        ScopedSocket scopedSocket(socket);
        // Surprise! We stop listening on socket accept..
        mServer->startListening();
        while (socketSendAll(scopedSocket.get(), mData.data(), mData.size()));
        return true;
    }

    int port() { return mPort; }

private:
    void loop() {
        while (!mDone) {
            mLooper.runWithDeadlineMs(mLooper.nowMs() + 100);
        }
    }

    int mPort;
    TestLooper mLooper;
    std::vector<uint8_t> mData;
    std::unique_ptr<AsyncSocketServer> mServer;
    std::unique_ptr<std::thread> mRunner;
    bool mDone = false;
};

class TestRunnerImpl final : public TestRunner::Service {
public:
    Status runTest(ServerContext* context,
                   const Test* request,
                   Test* response) override {
        // Note: Might not be perfect way of measuring cpu time.
        std::clock_t c_start = std::clock();
        switch (request->target()) {
            case Test::Nothing:
                response->set_chksum(request->chksum());
                break;
            case Test::SharedMemory:
                response->set_chksum(
                        prepare_shared_mem(request->handle(), request->size()));
                break;
            case Test::Grpc:
                response->set_chksum(prepare_grpc(request->size()));
                response->set_port(mService->port());
                break;
            case Test::RawSocket:
                response->set_chksum(prepare_socket(request->size()));
                response->set_port(mSrs->port());
                break;
            default:
                LOG(ERROR) << "Unknown test type.";
        }
        std::clock_t c_end = std::clock();
        response->set_cputime(c_end - c_start);
        return Status::OK;
    }

private:
    std::unique_ptr<SimpleReplySocket> mSrs;
    std::unique_ptr<SharedMemory> mWriter;
    std::unique_ptr<EmulatorControllerService> mService;

    // Calculate a simple hash of the memory region.
    // This forces a memory read on the region.
    uint64_t read_region(const uint8_t* src, size_t size) {
        return crc32(0, src, size);
    }

    uint64_t fill_region(uint8_t* dest, size_t size) {
        for (int i = 0; i < size; i++) {
            dest[i] = i % 256;
        }
        return read_region(dest, size);
    }

    // Calculate a simple hash from the shared memory region
    // in the given mode.
    uint64_t prepare_shared_mem(std::string handle, size_t size) {
        mWriter = std::make_unique<SharedMemory>(handle, size);
        auto user_read_only = 0600;
        int err = mWriter->create(user_read_only);

        uint8_t* wri = (uint8_t*)mWriter.get()->get();
        return fill_region(wri, size);
    }

    // Calculate the hash by receiving all bytes over a socket
    // and calculating the hash.
    uint64_t prepare_socket(size_t size) {
        std::vector<uint8_t> wri(size);
        auto chk = fill_region(wri.data(), wri.size());
        mSrs = std::make_unique<SimpleReplySocket>(wri);
        mSrs->startServer();
        return chk;
    }

    // Bring up a grpc endpoint that is ready to field requests.
    // it uses the same initialization as the emulator.
    uint64_t prepare_grpc(size_t size) {
        std::vector<uint8_t> wri(size);
        auto chk = fill_region(wri.data(), wri.size());

        TestEchoServiceImpl* echoService = new TestEchoServiceImpl();
        echoService->moveData(wri);

        EmulatorControllerService::Builder builder;
        mService = builder.withService(echoService)
                           .withPortRange(0, 1)
                           .withLogging(false)
                           .build();
        return chk;
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
