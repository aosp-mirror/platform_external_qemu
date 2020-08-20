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

// Calculate a simple hash from the shared memory region
// in the given mode.
uint64_t from_shared_mem(std::string handle, size_t size) {
    SharedMemory mReader(handle, size);
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
    auto status = client->data(&ctx, ::google::protobuf::Empty(), &response);
    return read_region((uint8_t*)response.data().data(),
                       response.data().size());
}

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
            LOG(INFO) << "Failed to launch " << executable << " --port "
                      << mPort << ", asuming you are running it manually.";
        }

        grpc::ChannelArguments ch_args;
        ch_args.SetMaxReceiveMessageSize(-1);
        mChannel = grpc::CreateCustomChannel(
                "localhost:" + std::to_string(mPort),
                ::grpc::InsecureChannelCredentials(), ch_args);
        mStub = TestRunner::NewStub(mChannel);
    }

    Test prepare(const Test test) {
        Test response;
        grpc::ClientContext ctx;
        auto status = mStub->runTest(&ctx, test, &response);
        if (!status.ok()) {
            LOG(FATAL) << "Failed to make remote call";
        }
        return response;
    }

private:
    grpc::ClientContext mCtx;
    System::Pid mTestPid;
    int mPort = 13121;
    std::unique_ptr<TestRunner::Stub> mStub;
    std::shared_ptr<grpc_impl::Channel> mChannel;
};

android::base::LazyInstance<GrpcDriver> sGrpcDriver = LAZY_INSTANCE_INIT;

Test prepare_remote_test(const Test tst) {
    return sGrpcDriver->prepare(tst);
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
    std::string unique_name = "file://" + android::base::PathUtils::join(
                                                  tempDir.path(), "shared.mem");
    const mode_t user_read_only = 0600;
    SharedMemory mWriter(unique_name, size);
    SharedMemory mReader(unique_name, size);

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

void BM_read_mmap_ext_chk(benchmark::State& state) {
    // Prepare a file backed memory region in the test process
    // and read it
    size_t size = state.range_x();
    TestTempDir tempDir{"shared"};
    std::string unique_name = "file://" + android::base::PathUtils::join(
                                                  tempDir.path(), "shared.mem");
    Test test;
    test.set_target(Test::SharedMemory);
    test.set_handle(unique_name);
    test.set_size(size);

    Test cfg = prepare_remote_test(test);
    uint64_t expected = cfg.chksum();
    while (state.KeepRunning()) {
        uint64_t chk = from_shared_mem(unique_name, size);
        if (chk != expected) {
            LOG(FATAL) << "Incorrect checksum: " << chk
                       << " != " << cfg.chksum();
        };
    };
    state.SetBytesProcessed(state.iterations() * size);
}

void BM_read_shared_ext_chk(benchmark::State& state) {
    // Prepare a shared memory region in the test process
    // and read it.
    size_t size = state.range_x();
    std::string unique_name = "memtest123";

    Test test;
    test.set_target(Test::SharedMemory);
    test.set_handle(unique_name);
    test.set_size(size);

    Test cfg = prepare_remote_test(test);
    uint64_t expected = cfg.chksum();
    while (state.KeepRunning()) {
        uint64_t chk = from_shared_mem(unique_name, size);
        if (chk != expected) {
            LOG(FATAL) << "Incorrect checksum: " << chk
                       << " != " << cfg.chksum();
        };
    };
    state.SetBytesProcessed(state.iterations() * size);
}

void BM_read_socket_ext_chk(benchmark::State& state) {
    // Prepare memory region in the test process
    // and read it by connecting to a raw socket retrieving
    // exactly "size" bytes.
    size_t size = state.range_x();

    Test test;
    test.set_target(Test::RawSocket);
    test.set_size(size);
    Test cfg = prepare_remote_test(test);
    uint64_t expected = cfg.chksum();
    while (state.KeepRunning()) {
        uint64_t chk = from_socket(cfg.port(), size);
        if (chk != expected) {
            LOG(FATAL) << "Incorrect checksum: " << chk
                       << " != " << cfg.chksum();
        };
    };
    state.SetBytesProcessed(state.iterations() * size);
}

void BM_read_grpc_ext_chk(benchmark::State& state) {
    // Prepare memory region in the test process
    // and read it by connecting to a the gRPC endpoint
    // to retrieve the data.

    size_t size = state.range_x();
    Test test;
    test.set_target(Test::Grpc);
    test.set_size(size);
    Test cfg = prepare_remote_test(test);
    uint64_t expected = cfg.chksum();
    auto address = "localhost:" + std::to_string(cfg.port());
    while (state.KeepRunning()) {
        uint64_t chk = from_grpc(address);
        if (chk != expected) {
            LOG(FATAL) << "Incorrect checksum: " << chk
                       << " != " << cfg.chksum();
        };
    };
    state.SetBytesProcessed(state.iterations() * size);
}

BASIC_BENCHMARK_TEST(BM_read_new_within_proc);  // Baseline malloc only.
BASIC_BENCHMARK_TEST(
        BM_read_mmap_within_proc);             // Baseline mmap in same process
BASIC_BENCHMARK_TEST(BM_read_socket_ext_chk);  // Baseline tcp/ip layer remote process.

BASIC_BENCHMARK_TEST(BM_read_mmap_ext_chk);
BASIC_BENCHMARK_TEST(BM_read_shared_ext_chk);
BASIC_BENCHMARK_TEST(BM_read_grpc_ext_chk);
BENCHMARK_MAIN()
