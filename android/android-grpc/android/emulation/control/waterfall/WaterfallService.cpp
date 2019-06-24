// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/waterfall/WaterfallService.h"

#include <istream>
#include <ostream>
#include <thread>

#include "android/base/Log.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/emulation/control/waterfall/BstrStreambuf.h"
#include "android/emulation/control/waterfall/SocketStreambuf.h"
#include "android/utils/sockets.h"
#include "grpcpp/create_channel_posix.h"

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("WaterfallService: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif
namespace android {
namespace emulation {
namespace control {

WaterfallService::WaterfallService(WaterfallConnection* con)
    : mWaterfallConnection(con) {
    mWaterfallConnection->initialize();
    mLooper = android::base::ThreadLooper::get();
}

std::unique_ptr<waterfall::Waterfall::Stub>
WaterfallService::getWaterfallStub() {
    auto channel = openChannel();
    if (!channel) {
        return nullptr;
    }
    return ::waterfall::Waterfall::NewStub(channel);
}

static void forwardStream(std::istream& in,
                          std::ostream& out,
                          int chunksize = 256) {
    char chunk[chunksize];
    while (!in.eof() && !out.eof()) {
        int fst = in.get();  // Blocks until at least on byte available.
        if (fst != SocketStreambuf::traits_type::eof()) {
            chunk[0] = (char)fst;
            int n = in.readsome(chunk + 1, sizeof(chunk) - 1) + 1;
            out.write(chunk, n);
        }
    }
}

static void forwardChannels(int grpcFd, int deviceFd) {
    std::thread fromOriginToDestination([grpcFd, deviceFd]() {
        SocketStreambuf sin(grpcFd);
        SocketStreambuf sout(deviceFd);
        BstrStreambuf bout(&sout);
        std::istream in(&sin);
        std::ostream out(&bout);
        forwardStream(in, out);
        socket_close(grpcFd);
        DD("forward complete, closed %d, errno: %d", grpcFd, errno);
    });
    std::thread fromDestinationToOrigin([grpcFd, deviceFd]() {
        SocketStreambuf sin(deviceFd);
        SocketStreambuf sout(grpcFd);
        BstrStreambuf bin(&sin);
        std::istream in(&bin);
        std::ostream out(&sout);
        forwardStream(in, out);
        socket_close(deviceFd);
        DD("forward complete, closed %d, errno: %d", deviceFd, errno);
    });

    // The threads will cleanup themselves, so we can just leave them.
    fromOriginToDestination.detach();
    fromDestinationToOrigin.detach();
}

// Opens a channel to the waterfall service inside the guest
// it will:
// handshake with the guest
// install encoders & decoders on sockets
// return an insecure grpc channel
std::shared_ptr<grpc::Channel> WaterfallService::openChannel() {
    int waterfallFd = mWaterfallConnection->openConnection();

    if (waterfallFd < 0) {
        LOG(ERROR) << "Unable to handshake with waterfall";
        return nullptr;
    }

    int internalFd;  // The fd that will forward to waterfallFd
    int externalFd;  // The fd that will be used for the grpc channel

    if (base::socketCreatePair(&internalFd, &externalFd) != 0) {
        LOG(ERROR) << "Unable to create translator pair";
        return nullptr;
    }
    DD("%d <-- device, grpc --> %d", internalFd, externalFd);
    mLooper->scheduleCallback([internalFd, waterfallFd]() {
        forwardChannels(internalFd, waterfallFd);
    });
    return grpc::CreateCustomInsecureChannelFromFd("waterfall-fwd-channel",
                                                   externalFd, {});
}

WaterfallService* getDefaultWaterfallService() {
    return new WaterfallService(getDefaultWaterfallConnection());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
