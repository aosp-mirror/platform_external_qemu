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
#include "android/emulation/control/AdbConnection.h"
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

static std::string serviceString(const char* serviceName,
                                 const ::waterfall::Cmd& cmd) {
    std::string service = serviceName;
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
    WaterfallAdbImpl() : mAdb(AdbConnection::connection()) {}

    Status ExecShellV2(
            ServerContext* context,
            ::grpc::ServerReaderWriter<::waterfall::CmdProgress,
                                       ::waterfall::CmdProgress>* stream,
            const ::waterfall::Cmd& cmd) {
        ::waterfall::CmdProgress incoming;
        auto adbStream = mAdb->open(serviceString("shell,v2,raw:", cmd));
        std::thread fromDestinationToOrigin([&stream, &adbStream, &context]() {
            bool ok = !context->IsCancelled() && adbStream->good();
            while (ok) {
                ::waterfall::CmdProgress outgoing;
                ShellHeader hdr;
                if (adbStream->read((char*)&hdr, sizeof(hdr))) {
                    std::vector<char> msg(hdr.length);
                    adbStream->read(msg.data(), hdr.length);

                    switch (hdr.id) {
                        case ShellHeader::kIdStderr:
                            outgoing.set_stderr(msg.data(), hdr.length);
                            break;
                        case ShellHeader::kIdStdout:
                            outgoing.set_stdout(msg.data(), hdr.length);
                            break;
                        case ShellHeader::kIdExit:
                            outgoing.set_exit_code(msg[0]);
                            break;
                        default:
                            // Ignonring the others..
                            LOG(WARNING) << "Don't know how to handle shell "
                                            "header of type: "
                                         << hdr.id;
                    }
                }
                ok = !context->IsCancelled() && stream->Write(outgoing) &&
                     adbStream->good();
            }
        });
        while (adbStream->good() && cmd.pipein() && stream->Read(&incoming)) {
            ShellHeader hdr{
                    .id = ShellHeader::kIdStdin,
                    .length = static_cast<uint32_t>(incoming.stdin().size())};
            adbStream->write((char*)&hdr, sizeof(hdr));
            adbStream->write(incoming.stdin().data(), hdr.length);
        };

        fromDestinationToOrigin.join();
        adbStream->close();
        return Status::OK;
    }

    Status Exec(ServerContext* context,
                ::grpc::ServerReaderWriter<::waterfall::CmdProgress,
                                           ::waterfall::CmdProgress>* stream)
            override {
        ::waterfall::CmdProgress fst;

        ::waterfall::CmdProgress incoming;
        if (mAdb->state() != AdbState::connected || !stream->Read(&incoming)) {
            // TODO(jansene): what error code?
            fst.set_exit_code(0xFFFF);
            return Status::OK;
        }

        if (mAdb->connection()->hasFeature("shell_v2"))
            return ExecShellV2(context, stream, incoming.cmd());

        auto cmd = incoming.cmd();
        auto adbStream = mAdb->open(serviceString("shell:", cmd));
        std::thread fromDestinationToOrigin([&stream, &adbStream, &context]() {
            bool ok = !context->IsCancelled() && adbStream->good();
            auto strmbuf = adbStream->rdbuf();
            while (ok) {
                ::waterfall::CmdProgress outgoing;

                // Max chunk size we are willing to collect before sending.
                int chunksize = 4096;
                char buf[chunksize];
                char* end = buf;
                std::string stdout;
                // Slurp up available bytes, worst case we send a single byte..
                // but this will be very responsive..
                if (strmbuf->in_avail() == 0) {
                    // Block and wait until we have 1 byte, (probably more, but
                    // we'll get to those)
                    if (adbStream->read(end, 1)) {
                        end += adbStream->gcount();
                        chunksize -= adbStream->gcount();
                    }
                }
                while (strmbuf->in_avail() > 0 && chunksize > 0) {
                    if (adbStream->read(end,
                                        std::min<size_t>(strmbuf->in_avail(),
                                                         chunksize))) {
                        end += adbStream->gcount();
                        chunksize -= adbStream->gcount();
                    }
                }

                outgoing.set_stdout(buf, end - buf);
                ok = !context->IsCancelled() && stream->Write(outgoing) &&
                     adbStream->good();
            }
        });

        while (adbStream->good() && cmd.pipein() && stream->Read(&incoming)) {
            adbStream->write(incoming.stdin().data(), incoming.stdin().size());
        };

        fromDestinationToOrigin.join();
        adbStream->close();
        return Status::OK;
    }

private:
    AdbConnection* mAdb;
};

waterfall::Waterfall::Service* getAdbWaterfallService() {
    return new WaterfallAdbImpl();
};

waterfall::Waterfall::Service* getWaterfallService(WaterfallProvider variant) {
    switch (variant) {
        case WaterfallProvider::adb:
            return getAdbWaterfallService();
        case WaterfallProvider::forward:
            return getWaterfallService();
    }
}

}  // namespace control
}  // namespace emulation
}  // namespace android
