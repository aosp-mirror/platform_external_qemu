/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <grpcpp/grpcpp.h>
#include "gnss_grpc_proxy.grpc.pb.h"

#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <common/libs/fs/shared_fd.h>

using gnss_grpc_proxy::GnssGrpcProxy;
using gnss_grpc_proxy::SendNmeaReply;
using gnss_grpc_proxy::SendNmeaRequest;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// Logic and data behind the server's behavior.
namespace cuttlefish {
class GnssGrpcProxyServiceImpl final : public GnssGrpcProxy::Service {
public:
    GnssGrpcProxyServiceImpl(cuttlefish::SharedFD gnss_in,
                             cuttlefish::SharedFD gnss_out,
                             std::string gnss_file_path);

    Status SendNmea(ServerContext* context,
                    const SendNmeaRequest* request,
                    SendNmeaReply* reply) override;

    void sendToSerial(const char* data = nullptr, int len = 0);

    void StartServer();

    void StartReadFileThread();

    void ReadNmeaFromLocalFile();

    void oneShotSendNmea(const char* data, int len, const char* sep);

private:
    static constexpr char CMD_GET_LOCATION[] = "CMD_GET_LOCATION";
    static constexpr uint32_t GNSS_SERIAL_BUFFER_SIZE = 4096;

private:
    [[noreturn]] void ReadLoop();

    cuttlefish::SharedFD gnss_in_;
    cuttlefish::SharedFD gnss_out_;

    std::string gnss_file_path_;
    std::thread read_thread_;
    std::thread file_read_thread_;
    std::string cached_nmea;
    std::string legacy_nmea;  // from console and GUI
    std::mutex cached_nmea_mutex;
};

}  // namespace cuttlefish
