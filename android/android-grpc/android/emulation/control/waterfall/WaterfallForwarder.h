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
#pragma once

#include <grpcpp/grpcpp.h>

#include <thread>

#include "android/emulation/control/waterfall/WaterfallServiceLibrary.h"
#include "waterfall.grpc.pb.h"  // Needed for ServerReaderWriter<T, T>

#define FWD_DEBUG 0
#if FWD_DEBUG >= 1
#define DD(fmt, ...) \
    printf("WaterfallFwd: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(fmt, ...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {
using grpc::Status;

// Many -> Many
template <class T>
Status forwardCall(grpc::ServerReaderWriter<T, T>* origin,
                   grpc::ClientReaderWriter<T, T>* destination) {
    bool originAlive = true;
    std::thread fromOriginToDestination([&origin, &destination]() {
        T msg;
        while (origin->Read(&msg) && destination->Write(msg)) {
        }
        DD("orig --> dest completed..");
        destination->WritesDone();
    });
    std::thread fromDestinationToOrigin([&origin, &originAlive, &destination]() {
        T msg;
        while (originAlive && destination->Read(&msg)) {
            originAlive = origin->Write(msg);
        }
        DD("dest --> orig completed..");
    });
    fromOriginToDestination.join();
    fromDestinationToOrigin.join();
    if (!originAlive) {
        // destination is still giving us things.. and we are dead.
        return Status::CANCELLED;
    }
    DD("Sending finish to destination.");
    return destination->Finish();
}

// 1 -> Many
template <class T>
Status forwardCall(grpc::ServerWriter<T>* origin,
                   grpc::ClientReader<T>* destination) {
    T msg;
    while (destination->Read(&msg)) {
        DD("1 -- Many");
        origin->Write(msg);
    }
    DD("1 --> Many: completed");
    return destination->Finish();
}

// Many -> 1
template <class T>
Status forwardCall(grpc::ServerReader<T>* origin,
                   grpc::ClientWriter<T>* destination) {
    T msg;
    int i = 0;
    while (origin->Read(&msg)) {
        destination->Write(msg);
    }
    DD("Many --> 1: writes done!");
    destination->WritesDone();
    DD("Many --> 1: Finish");
    return destination->Finish();
}

// Forwards a waterfall call to the waterfall client, it will:
// - Writes the message: pipe:unix:<soname> to fd.
// - The client will open up a new connection to the given unix <soname>
// (let's call it fin)
// - The message "rdy" need to be written to fin
// - The message "rdy" needs to be read from fin.
// - An encoder needs to be installed that encode bytes going to fin
// - A decoder needs to be installed that decodes bytes coming from fin.
//
// The wire format is:
//
// 1. Length prefix. A four-byte integer that contains the number of bytes
//    in the following data string. It appears immediately before the first
//    character of the data string. The integer is encoded in little endian.
// 2. Data string.
//
// Usage:
//
//     return forwardWaterfallCall<::waterfall::Message>(
//                &mWaterfall, stream,
//                [](auto stub, auto ctx) { return stub->Echo(ctx); });
//
// Note you will have to replace ::waterfall::Message with the actual type
// and stub->XXX with the actual method you wish to forward.
template <class ServerType, class ClientType>
Status forwardWaterfallCall(WaterfallServiceLibrary* service,
                            ServerType* origin,
                            std::function<std::unique_ptr<ClientType>(
                                    waterfall::Waterfall::Stub* stub,
                                    grpc::ClientContext* ctx)> fwdMethod) {
    ScopedWaterfallStub fwd(service);
    auto waterfall = fwd.get();
    if (!waterfall) {
        return Status(grpc::StatusCode::UNAVAILABLE,
                      "Unable to reach waterfall");
    }
    grpc::ClientContext deviceContext;
    auto destination = fwdMethod(waterfall, &deviceContext);
    auto res = forwardCall(origin, destination.get());
    DD("Completed forward %s", res.error_message().c_str());
    return res;
}

template <class T>
const auto StreamingToStreaming = forwardWaterfallCall<grpc::ServerReaderWriter<T, T>,
                                             grpc::ClientReaderWriter<T, T>>;

template <class T>
const auto UnaryToStreaming =
        forwardWaterfallCall<grpc::ServerWriter<T>, grpc::ClientReader<T>>;

template <class T>
const auto StreamingToUnary =
        forwardWaterfallCall<grpc::ServerReader<T>, grpc::ClientWriter<T>>;

}  // namespace control
}  // namespace emulation
}  // namespace android