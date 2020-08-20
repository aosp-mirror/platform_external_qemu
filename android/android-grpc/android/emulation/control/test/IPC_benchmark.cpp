// Copyright 2020 The Android Open Source Project
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

#include <assert.h>         // for assert
#include <errno.h>          // for errno
#include <grpcpp/grpcpp.h>  // for CreateCu...
#include <stdio.h>          // for size_t
#include <stdlib.h>         // for exit
#include <sys/types.h>      // for mode_t
#include <zlib.h>           // for crc32

#include <cstdint>     // for uint8_t
#include <functional>  // for __bind
#include <memory>      // for unique_ptr
#include <ostream>     // for operator<<
#include <string>      // for string
#include <thread>      // for thread
#include <vector>      // for vector

#include "android/base/Log.h"                                // for LogStrea...
#include "android/base/StringView.h"                         // for StringView
#include "android/base/async/AsyncSocketServer.h"            // for AsyncSoc...
#include "android/base/files/PathUtils.h"                    // for PathUtils
#include "android/base/memory/LazyInstance.h"                // for LazyInst...
#include "android/base/memory/SharedMemory.h"                // for SharedMe...
#include "android/base/sockets/ScopedSocket.h"               // for ScopedSo...
#include "android/base/sockets/SocketUtils.h"                // for socketSe...
#include "android/base/system/System.h"                      // for System
#include "android/base/testing/TestLooper.h"                 // for TestLooper
#include "android/base/testing/TestTempDir.h"                // for TestTempDir
#include "android/emulation/control/GrpcServices.h"          // for Emulator...
#include "android/emulation/control/test/TestEchoService.h"  // for TestEcho...
#include "benchmark/benchmark_api.h"                         // for Benchmark
#include "grpcpp/security/credentials.h"                     // for Insecure...
#include "grpcpp/support/channel_arguments.h"                // for ChannelA...
#include "ipc_test_service.grpc.pb.h"                        // for TestRunn...
#include "ipc_test_service.pb.h"                             // for Test

namespace grpc_impl {
class Channel;
}  // namespace grpc_impl

// Let's test upto 32 mb, we want real time (no cpu time as we are using
// a remote process, cpu time becomes meaningless)
#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)                \
            ->Arg(1 << 20)      \
            ->Arg(1 << 21)      \
            ->Arg(1 << 22)      \
            ->Arg(1 << 23)      \
            ->Arg(1 << 24)      \
            ->Arg(1 << 25)      \
            ->Threads(1)        \
            ->UseRealTime()

using namespace android::base;
using namespace android::emulation::control;

uint64_t fill_region(uint8_t* dest, size_t size) {
    for (int i = 0; i < size; i++) {
        dest[i] = i % 256;
    }
    return crc32(0, dest, size);
}

uint64_t read_region(const uint8_t* src, size_t size) {
    return crc32(0, src, size);
}

const std::string kSharedReader = "ipc_reader";

struct SharedMemConfig {
    std::string handle = "";
    SharedMemory::ShareType mode = SharedMemory::ShareType::FILE_BACKED;
};

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
        if (!socketSendAll(scopedSocket.get(), mData.data(), mData.size())) {
            LOG(ERROR) << "Failed to send all the bytes: " << errno;
        }
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

class GrpcDriver {
public:
    GrpcDriver() { launchRemoteProc(); }
    ~GrpcDriver() { System::get()->killProcess(mTestPid); }

    void launchRemoteProc() {
        std::string executable =
                System::get()->findBundledExecutable(kSharedReader);

        if (!System::get()->runCommand(
                    {executable, "--port", std::to_string(mPort)},
                    RunOptions::DontWait, System::kInfinite, nullptr,
                    &mTestPid)) {
            LOG(FATAL) << "Failed to launch " << executable << " --port "
                       << mPort << ", make sure the port is free!";
        }

        grpc::ChannelArguments ch_args;
        ch_args.SetMaxReceiveMessageSize(-1);
        mChannel = grpc::CreateCustomChannel(
                "localhost:" + std::to_string(mPort),
                ::grpc::InsecureChannelCredentials(), ch_args);
        mStub = TestRunner::NewStub(mChannel);
    }

    void runTest(benchmark::State& state, const Test test) {
        while (state.KeepRunning()) {
            runSingleTest(state, test);
        };

        if (test.size() > 0)
            state.SetBytesProcessed(state.iterations() * test.size());
    }

private:
    void runSingleTest(benchmark::State& state, const Test& test) {
        Test response;
        grpc::ClientContext ctx;
        auto status = mStub->runTest(&ctx, test, &response);
        if (!status.ok()) {
            LOG(FATAL) << "Failed to make remote call";
        }
        if (test.target() != Test::Nothing) {
            if (response.chksum() != test.chksum()) {
                LOG(FATAL) << "Invalid checksm " << test.chksum()
                           << " != " << response.chksum();
            }
        }
        // state.SetIterationTime(response.cputime());
    }

    grpc::ClientContext mCtx;
    System::Pid mTestPid;
    int mPort = 13121;
    std::unique_ptr<TestRunner::Stub> mStub;
    std::shared_ptr<grpc_impl::Channel> mChannel;
};

android::base::LazyInstance<GrpcDriver> sGrpcDriver = LAZY_INSTANCE_INIT;

void run_remote_test(benchmark::State& state, const Test tst) {
    sGrpcDriver->runTest(state, tst);
}

void do_test(benchmark::State& state, uint8_t* dst, uint8_t* src, size_t size) {
    auto wri = fill_region(dst, size);
    while (state.KeepRunning()) {
        auto rea = read_region(src, size);
        if (wri != rea) {
            LOG(FATAL) << "Invalid checksm " << wri << " != " << rea;
        }
    }
    state.SetBytesProcessed(state.iterations() * size * sizeof(char));
}

void BM_read_mmap_within_proc(benchmark::State& state) {
    // Write incrementing number to a shared region and validate
    // the checksum by reading it in a shared region in a different
    // location.
    size_t size = state.range_x();

    TestTempDir tempDir("shared");
    std::string unique_name =
            android::base::PathUtils::join(tempDir.path(), "shared.mem");
    const mode_t user_read_only = 0600;
    SharedMemory mWriter(unique_name, size,
                         SharedMemory::ShareType::FILE_BACKED);
    SharedMemory mReader(unique_name, size,
                         SharedMemory::ShareType::FILE_BACKED);

    int err = mWriter.create(user_read_only);
    assert(err == 0);
    err = mReader.open(SharedMemory::AccessMode::READ_ONLY);
    assert(err == 0);
    uint8_t* wri = (uint8_t*)mWriter.get();
    uint8_t* rea = (uint8_t*)mReader.get();
    assert(wri != rea);
    do_test(state, wri, rea, size);
}

void BM_read_new_within_proc(benchmark::State& state) {
    // Write incrementing number to a "malloced" region and validate
    // the checksum by reading it from the same region
    size_t size = state.range_x();
    std::vector<uint8_t> region(size);
    do_test(state, region.data(), region.data(), size);
}

void BM_ext_nop(benchmark::State& state) {
    // Invoke nothing in the remote tester.
    // Use this to obtain a baseline for cost of making a gRPC call
    // to schedule an actual test.
    Test test;
    test.set_target(Test::Nothing);
    run_remote_test(state, test);
}

void BM_read_mmap_ext_chk(benchmark::State& state) {
    // Write incrementing number to a shared region, and launch an ext
    // process to verify the checksum.
    size_t size = state.range_x();

    TestTempDir tempDir{"shared"};
    std::string unique_name =
            android::base::PathUtils::join(tempDir.path(), "shared.mem");
    const mode_t user_read_only = 0600;
    SharedMemory mWriter(unique_name, size,
                         SharedMemory::ShareType::FILE_BACKED);
    int err = mWriter.create(user_read_only);

    uint8_t* wri = (uint8_t*)mWriter.get();
    auto chk = fill_region(wri, size);
    Test test;
    test.set_target(Test::SharedMemory);
    test.set_handle(unique_name);
    test.set_mode(Test::File);
    test.set_size(size);
    test.set_chksum(chk);
    run_remote_test(state, test);
}

void BM_read_shared_ext_chk(benchmark::State& state) {
    // Write incrementing number to a shared region, and launch an ext
    // process to verify the checksum.
    size_t size = state.range_x();

    std::string unique_name = "memtest123";
    const mode_t user_read_only = 0600;
    SharedMemory mWriter(unique_name, size);
    int err = mWriter.create(user_read_only);

    uint8_t* wri = (uint8_t*)mWriter.get();
    auto chk = fill_region(wri, size);

    Test test;
    test.set_target(Test::SharedMemory);
    test.set_handle(unique_name);
    test.set_mode(Test::Shared);
    test.set_size(size);
    test.set_chksum(chk);
    run_remote_test(state, test);
}

void BM_read_socket_ext_chk(benchmark::State& state) {
    // Write incrementing number to a socket, and launch an ext
    // process to verify the checksum.
    size_t size = state.range_x();
    std::vector<uint8_t> wri(size);
    auto chk = fill_region(wri.data(), wri.size());
    auto ecq = read_region(wri.data(), wri.size());
    if (chk != ecq) {
        exit(1);
    }
    SimpleReplySocket srs(wri);
    srs.startServer();

    Test test;
    test.set_target(Test::RawSocket);
    test.set_size(size);
    test.set_chksum(chk);
    test.set_port(srs.port());
    run_remote_test(state, test);
}

void BM_read_grpc_ext_chk(benchmark::State& state) {
    // Write incrementing number to a shared region, read back the shared
    // region in a different process by calling the gRPC endpoint in this process.
    size_t size = state.range_x();
    std::vector<uint8_t> wri(size);
    auto chk = fill_region(wri.data(), wri.size());

    TestEchoServiceImpl* echoService = new TestEchoServiceImpl();
    echoService->moveData(wri);

    EmulatorControllerService::Builder builder;
    auto ecs = builder.withService(echoService)
                       .withPortRange(0, 1)
                       .withLogging(false)
                       .build();

    Test test;
    test.set_target(Test::Grpc);
    test.set_size(size);
    test.set_chksum(chk);
    test.set_port(ecs->port());
    run_remote_test(state, test);
}

// BASIC_BENCHMARK_TEST(BM_read_new_within_proc); // Baseline malloc only.
// BASIC_BENCHMARK_TEST(BM_read_mmap_within_proc); // Baseline mmap in side proc
// BASIC_BENCHMARK_TEST(BM_read_socket_ext_chk); // Baseline tcp/ip layer.

BENCHMARK(BM_ext_nop);  // Warmup, overhead of a grpc call.
BASIC_BENCHMARK_TEST(BM_read_mmap_ext_chk);
BASIC_BENCHMARK_TEST(BM_read_shared_ext_chk);
BASIC_BENCHMARK_TEST(BM_read_grpc_ext_chk);
BENCHMARK_MAIN()
