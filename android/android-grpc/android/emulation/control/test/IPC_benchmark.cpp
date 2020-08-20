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
#include <grpcpp/grpcpp.h>  // for CreateCustomChannel
#include <stdio.h>          // for size_t
#include <sys/types.h>      // for mode_t
#include <zlib.h>           // for crc32

#include <cstdint>  // for uint8_t, uint64_t
#include <memory>   // for unique_ptr, shar...
#include <ostream>  // for operator<<, basi...
#include <string>   // for string, operator+
#include <vector>   // for vector

#include "android/base/Log.h"                        // for LogStreamVoidify
#include "android/base/StringView.h"                 // for StringView
#include "android/base/files/PathUtils.h"            // for PathUtils
#include "android/base/memory/LazyInstance.h"        // for LazyInstance
#include "android/base/memory/SharedMemory.h"        // for SharedMemory
#include "android/base/sockets/ScopedSocket.h"       // for ScopedSocket
#include "android/base/sockets/SocketUtils.h"        // for socketRecvAll
#include "android/base/system/System.h"              // for System, RunOptions
#include "android/base/testing/TestTempDir.h"        // for TestTempDir
#include "android/emulation/control/GrpcServices.h"  // for control
#include "benchmark/benchmark_api.h"                 // for State, Benchmark
#include "google/protobuf/empty.pb.h"                // for Empty
#include "grpcpp/security/credentials.h"             // for InsecureChannelC...
#include "grpcpp/support/channel_arguments.h"        // for ChannelArguments
#include "ipc_test_service.grpc.pb.h"                // for TestRunner::Stub
#include "ipc_test_service.pb.h"                     // for Test, Test::Grpc
#include "test_echo_service.grpc.pb.h"               // for TestEcho, TestEc...
#include "test_echo_service.pb.h"                    // for Msg

// This contains a series of benchmarks that can be used to determine which mode
// of ipc is best suited for sharing large blobs of memory (i.e. image frame
// from android device) to a separate process (android studio)
//
// The tests do the following:
//
// - Launch a benchmarking process (A) (this program)
// - Launch a testing process (B), accessible via gRPC (you might need to run
// this yourself)
//
// The tests basically ask process B to prepare a chunk of memory and
// retrieve the crc32. Next we measure how fast A can retrieve the data
// from B.
//
// The following mechanisms of sharing can be measured:
//
// - Shared memory using /dev/shm
// - Memory mapped files using a tmp file
// - Retrieving the blob of memory via gRPC
// - Retrieving the blob of memory by directly reading it from a socket.
//
// The tests always do this:
//
// - Configure the shared object in process B
// - Setup communication channel
// - Read bytes
// - Calculate CRC32
// - Return checksum.
//
// We always measure wall time.

// Let's test upto 32 mb, we want real time  as we are using
// a remote process, cpu time becomes meaningless, we really want to know
// how long it takes for all the bytes to have been read.
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

// This class prepares the remote process for the test we
// are going to run.
class GrpcDriver {
public:
    GrpcDriver() { launchRemoteProc(); }
    ~GrpcDriver() { System::get()->killProcess(mTestPid); }

    void launchRemoteProc() {
        // Do your best to find the reader executable, does not
        // always work. You can always launch manually.
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
        auto channel = grpc::CreateCustomChannel(
                "localhost:" + std::to_string(mPort),
                ::grpc::InsecureChannelCredentials(), ch_args);
        mStub = TestRunner::NewStub(channel);
    }

    Test prepare(const Test test) {
        Test response;
        grpc::ClientContext ctx;
        auto status = mStub->runTest(&ctx, test, &response);
        if (!status.ok()) {
            LOG(FATAL) << "Failed to make configure remote process: "
                       << status.error_code() << ", " << status.error_message();
        }
        return response;
    }

private:
    System::Pid mTestPid;
    int mPort = 13121;
    std::unique_ptr<TestRunner::Stub> mStub;
};

// We only need one driver.
android::base::LazyInstance<GrpcDriver> sGrpcDriver = LAZY_INSTANCE_INIT;

// This is the base class for every individual ipc based perfmTest.
// you will probably want to subclass this and implement the
// prepare and chksum methods.
class PerfTest {
public:
    PerfTest() {}
    ~PerfTest() {}

    // Called to prepare your mechanism for IPC.
    // This will be called at least once per mTest.
    // Here you setup your IPC mechanism, socket, shared memory
    // etc..
    virtual void prepare() = 0;

    // Called to actually calculate the checksum
    virtual uint64_t chksum() = 0;

    // Called to get the expected checksum
    virtual uint64_t expected() { return mConfig.chksum(); }

    // Configure the remote process for the upcoming test
    void setup(uint64_t size) {
        mTest.set_size(size);
        mConfig = sGrpcDriver->prepare(mTest);
    }

protected:
    Test mTest;    // Desired test configuration.
    Test mConfig;  // Actual configuration after remote prepare was called.
};

class SharedMemoryTest : public PerfTest {
public:
    SharedMemoryTest(std::string unique_name) {
        mTest.set_target(Test::SharedMemory);
        mTest.set_handle(unique_name);
    }

    SharedMemoryTest() {
        // File backed ram in a temporary file somewhere.
        mTest.set_target(Test::SharedMemory);
        mTest.set_handle("file://" + android::base::PathUtils::join(
                                             mTempDir.path(), "shared.mem"));
    }

    void prepare() override {
        mMemory = std::make_unique<SharedMemory>(mTest.handle(), mTest.size());
        mMemory->open(SharedMemory::AccessMode::READ_ONLY);
    }

    uint64_t chksum() override {
        return read_region((uint8_t*)mMemory->get(), mTest.size());
    }

private:
    TestTempDir mTempDir{"shared"};
    std::unique_ptr<SharedMemory> mMemory;
};

class SocketTest : public PerfTest {
public:
    SocketTest() { mTest.set_target(Test::RawSocket); }

    void prepare() override {
        mData.resize(mTest.size());
        mSocket = socketTcp4LoopbackClient(mConfig.port());
    }

    uint64_t chksum() override {
        if (!socketRecvAll(mSocket.get(), mData.data(), mData.size())) {
            LOG(ERROR)
                    << "Did not receive all bytes from remote process at port: "
                    << mConfig.port();
            return -1;
        }
        return read_region(mData.data(), mData.size());
    }

private:
    std::vector<uint8_t> mData;
    ScopedSocket mSocket;
};

class GrpcTest : public PerfTest {
public:
    GrpcTest() { mTest.set_target(Test::Grpc); }

    void prepare() override {
        auto address = "localhost:" + std::to_string(mConfig.port());
        grpc::ChannelArguments ch_args;
        ch_args.SetMaxReceiveMessageSize(-1);
        auto channel = grpc::CreateCustomChannel(
                address, ::grpc::InsecureChannelCredentials(), ch_args);
        mStub = TestEcho::NewStub(channel);
    }

    uint64_t chksum() override {
        grpc::ClientContext ctx;
        Msg response;
        mStub->data(&ctx, ::google::protobuf::Empty(), &response);
        return read_region((uint8_t*)response.data().data(),
                           response.data().size());
    }

private:
    std::unique_ptr<TestEcho::Stub> mStub;
};

class LocalTest : public PerfTest {
public:
    LocalTest() { mTest.set_target(Test::Nothing); }

    void prepare() override {
        mData.resize(mTest.size());
        mConfig.set_chksum(fill_region(mData.data(), mData.size()));
    }

    uint64_t chksum() override {
        return read_region(mData.data(), mData.size());
    }

private:
    std::vector<uint8_t> mData;
};

// Actual test runner, call setup, prepare and checksum when needed.
void do_test(PerfTest* perf,
             benchmark::State& state,
             bool prepare_only_once = false) {
    perf->setup(state.range_x());
    if (prepare_only_once) {
        perf->prepare();
    }
    while (state.KeepRunning()) {
        if (!prepare_only_once) {
            perf->prepare();
        }
        uint64_t chk = perf->chksum();
        if (chk != perf->expected()) {
            LOG(FATAL) << "Incorrect checksum: " << chk
                       << " != " << perf->expected();
        };
    };

    state.SetBytesProcessed(state.iterations() * state.range_x());
}

void BM_read_new_within_proc(benchmark::State& state) {
    // Write incrementing number to a "malloced" region and validate
    // the checksum by reading it from the same region
    auto mem = LocalTest();
    do_test(&mem, state, true);
}

void BM_read_mmap_ext_chk(benchmark::State& state) {
    // Prepare a file backed memory region in the test process
    // and read it
    auto shm = SharedMemoryTest();
    do_test(&shm, state);
}

void BM_read_mmap_reuse_ext_chk(benchmark::State& state) {
    // Prepare a file backed memory region in the test process
    // and read it, initializing only once.
    auto shm = SharedMemoryTest();
    do_test(&shm, state, true);
}

void BM_read_shared_ext_chk(benchmark::State& state) {
    // Prepare a shared memory region in the test process
    // and read it.
    auto shm = SharedMemoryTest("memtest123");
    do_test(&shm, state);
}

void BM_read_shared_reuse_ext_chk(benchmark::State& state) {
    // Prepare a shared memory region in the test process
    // and read it.
    auto shm = SharedMemoryTest("memtest123");
    do_test(&shm, state, true);
}

void BM_read_socket_ext_chk(benchmark::State& state) {
    // Prepare memory region in the test process
    // and read it by connecting to a raw socket retrieving
    // exactly "size" bytes.
    auto sock = SocketTest();
    do_test(&sock, state);
}

void BM_read_socket_reuse_ext_chk(benchmark::State& state) {
    // Prepare memory region in the test process
    // and read it by connecting to a raw socket retrieving
    // exactly "size" bytes.
    auto sock = SocketTest();
    do_test(&sock, state, true);
}

void BM_read_grpc_ext_chk(benchmark::State& state) {
    // Prepare memory region in the test process
    // and read it by connecting to a the gRPC endpoint
    // to retrieve the data.
    auto grpc = GrpcTest();
    do_test(&grpc, state);
}

void BM_read_grpc_reuse_ext_chk(benchmark::State& state) {
    // Prepare memory region in the test process
    // and read it by connecting to a the gRPC endpoint
    // to retrieve the data.
    auto grpc = GrpcTest();
    do_test(&grpc, state, true);
}

BASIC_BENCHMARK_TEST(BM_read_new_within_proc);  // Baseline malloc only.
BASIC_BENCHMARK_TEST(BM_read_socket_ext_chk);   // Baseline tcp/ip layer
BASIC_BENCHMARK_TEST(BM_read_socket_reuse_ext_chk);
BASIC_BENCHMARK_TEST(BM_read_shared_ext_chk);  // /dev/shm
BASIC_BENCHMARK_TEST(BM_read_shared_reuse_ext_chk);
BASIC_BENCHMARK_TEST(BM_read_mmap_ext_chk);  // memory mapped files.
BASIC_BENCHMARK_TEST(BM_read_mmap_reuse_ext_chk);
BASIC_BENCHMARK_TEST(BM_read_grpc_ext_chk);  // grpc endpoint.
BASIC_BENCHMARK_TEST(BM_read_grpc_reuse_ext_chk);
BENCHMARK_MAIN()
