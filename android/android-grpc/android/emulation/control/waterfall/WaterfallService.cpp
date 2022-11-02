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
#include <functional>
#include <memory>

#include "aemu/base/Log.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/emulation/control/waterfall/SocketController.h"
#include "android/emulation/control/waterfall/WaterfallForwarder.h"
#include "android/emulation/control/waterfall/WaterfallServiceLibrary.h"
#include "waterfall.grpc.pb.h"

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google
namespace waterfall {
class CmdProgress;
class ForwardMessage;
class Message;
class Transfer;
class VersionMessage;
}  // namespace waterfall

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using namespace android::base;
using namespace android::control::interceptor;

namespace android {
namespace emulation {
namespace control {

class WaterfallImpl final : public waterfall::Waterfall::Service {
public:
    WaterfallImpl() : mWaterfall(new ControlSocketLibrary()) {}

    Status Forward(ServerContext* context,
                   ::grpc::ServerReaderWriter<::waterfall::ForwardMessage,
                                              ::waterfall::ForwardMessage>*
                           stream) override {
        return StreamingToStreaming<::waterfall::ForwardMessage>(
                mWaterfall.get(), stream,
                [](auto stub, auto ctx) { return stub->Forward(ctx); });
    }

    Status Echo(
            ServerContext* context,
            ::grpc::ServerReaderWriter<::waterfall::Message,
                                       ::waterfall::Message>* stream) override {
        return StreamingToStreaming<::waterfall::Message>(
                mWaterfall.get(), stream,
                [](auto stub, auto ctx) { return stub->Echo(ctx); });
    }

    Status Exec(ServerContext* context,
                ::grpc::ServerReaderWriter<::waterfall::CmdProgress,
                                           ::waterfall::CmdProgress>* stream)
            override {
        return StreamingToStreaming<::waterfall::CmdProgress>(
                mWaterfall.get(), stream,
                [](auto stub, auto ctx) { return stub->Exec(ctx); });
    }

    Status Pull(ServerContext* context,
                const ::waterfall::Transfer* request,
                ServerWriter<::waterfall::Transfer>* writer) override {
        return UnaryToStreaming<::waterfall::Transfer>(
                mWaterfall.get(), writer, [&request](auto stub, auto ctx) {
                    return stub->Pull(ctx, *request);
                });
    }

    Status Push(ServerContext* context,
                grpc::ServerReader<::waterfall::Transfer>* reader,
                ::waterfall::Transfer* reply) override {
        return StreamingToUnary<::waterfall::Transfer>(
                mWaterfall.get(), reader, [&reply](auto stub, auto ctx) {
                    return stub->Push(ctx, reply);
                });
    }

    Status Version(ServerContext* context,
                   const ::google::protobuf::Empty* request,
                   ::waterfall::VersionMessage* reply) override {
        ScopedWaterfallStub fwd(mWaterfall.get());
        if (!fwd.get())
            return Status(grpc::StatusCode::UNAVAILABLE,
                          "Unable to reach waterfall");

        ::grpc::ClientContext defaultCtx;
        return fwd->Version(&defaultCtx, *request, reply);
    }

private:
    std::unique_ptr<WaterfallServiceLibrary> mWaterfall;
};


grpc::Service* getWaterfallService() {
    return new WaterfallImpl();
};


}  // namespace control
}  // namespace emulation
}  // namespace android
