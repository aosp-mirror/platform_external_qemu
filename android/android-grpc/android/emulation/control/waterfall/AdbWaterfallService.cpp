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
#include <thread>

#include "android/base/Log.h"
#include "android/emulation/control/adb/AdbConnection.h"
#include "android/emulation/control/adb/AdbShellStream.h"
#include "android/emulation/control/waterfall/WaterfallService.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
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

namespace android {
namespace emulation {
namespace control {

static std::string shellStr(const ::waterfall::Cmd& cmd) {
    std::string service = "";
    int argIdx = 1;

    // Waterfall likes to executes commands in a subshell..
    // which doesn't work with adb as expected, so let's not do that.
    if (cmd.path() != "/system/bin/sh") {
        service += cmd.path();
        argIdx = 0;
    }

    for (; argIdx < cmd.args_size(); argIdx++) {
        service.append(cmd.args(argIdx));
        service.append(" ");
    }

    return service;
}

class WaterfallAdbImpl final : public waterfall::Waterfall::Service {
public:
    WaterfallAdbImpl() = default;

    Status Exec(ServerContext* context,
                ::grpc::ServerReaderWriter<::waterfall::CmdProgress,
                                           ::waterfall::CmdProgress>* stream)
            override {
        ::waterfall::CmdProgress fst;

        ::waterfall::CmdProgress incoming;
        // We are willing to wait 200ms to establish a connection if none exists.
        auto adb = AdbConnection::connection(2000);
        if (adb->state() != AdbState::connected || !stream->Read(&incoming)) {
            // TODO(jansene): what error code?
            fst.set_exit_code(0xFFFF);
            return Status::OK;
        }

        auto cmd = incoming.cmd();
        AdbShellStream shell(shellStr(cmd));
        std::thread fromDestinationToOrigin([&shell, &stream, &context]() {
            bool ok = !context->IsCancelled() && shell.good();
            int exitcode = 0;
            std::vector<char> sout;
            std::vector<char> serr;
            while (ok) {
                ::waterfall::CmdProgress outgoing;
                sout.clear();
                serr.clear();
                shell.read(sout, serr, exitcode);
                outgoing.set_stdout(sout.data(), sout.size());
                outgoing.set_stderr(serr.data(), serr.size());
                outgoing.set_exit_code(exitcode);
                ok = stream->Write(outgoing) && !context->IsCancelled() &&
                     shell.good();
            }
        });

        while (cmd.pipein() && stream->Read(&incoming)) {
            shell.write(incoming.stdin());
        };

        fromDestinationToOrigin.join();
        return Status::OK;
    }
};  // namespace control

waterfall::Waterfall::Service* getAdbWaterfallService() {
    return new WaterfallAdbImpl();
};

waterfall::Waterfall::Service* getWaterfallService(WaterfallProvider variant) {
    switch (variant) {
        case WaterfallProvider::adb:
            return getAdbWaterfallService();
        case WaterfallProvider::forward:
            return getWaterfallService();
        default:
            return nullptr;
    }
}

}  // namespace control
}  // namespace emulation
}  // namespace android
