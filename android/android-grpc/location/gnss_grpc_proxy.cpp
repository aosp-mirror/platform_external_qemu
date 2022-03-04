/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "gnss_grpc_proxy.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "grpcpp.h"

#include <android-base/logging.h>
#include <android/base/files/PathUtils.h>
#include <common/libs/fs/shared_select.h>
#include "gnss_grpc_proxy.grpc.pb.h"

#include <signal.h>

#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

using gnss_grpc_proxy::GnssGrpcProxy;
using gnss_grpc_proxy::SendNmeaReply;
using gnss_grpc_proxy::SendNmeaRequest;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using android::base::PathUtils;

namespace cuttlefish {

static GnssGrpcProxyServiceImpl* s_proxy = nullptr;

GnssGrpcProxyServiceImpl::GnssGrpcProxyServiceImpl(
        cuttlefish::SharedFD gnss_in,
        cuttlefish::SharedFD gnss_out,
        std::string gnss_file_path)
    :

      gnss_in_(gnss_in),
      gnss_out_(gnss_out),
      gnss_file_path_(gnss_file_path) {
    s_proxy = this;
}

Status GnssGrpcProxyServiceImpl::SendNmea(ServerContext* context,
                                          const SendNmeaRequest* request,
                                          SendNmeaReply* reply) {
    reply->set_reply("Received nmea record.");

    auto buffer = request->nmea();
    std::lock_guard<std::mutex> lock(cached_nmea_mutex);
    cached_nmea = request->nmea();
    return Status::OK;
}

void GnssGrpcProxyServiceImpl::sendToSerial(const char* data, int len) {
    std::lock_guard<std::mutex> lock(cached_nmea_mutex);
    if (data && len > 0)
        cached_nmea.assign(data, len);
    int bytes_written = cuttlefish::WriteAll(gnss_in_, cached_nmea);
    if (bytes_written < 0) {
        LOG(ERROR) << "Error writing to fd: " << gnss_in_->StrError();
    }
}

void GnssGrpcProxyServiceImpl::StartServer() {
    // Create a new thread to handle writes to the gnss and to the any client
    // connected to the socket.
    read_thread_ = std::thread([this]() { ReadLoop(); });
}

void GnssGrpcProxyServiceImpl::StartReadFileThread() {
    // Create a new thread to handle writes to the gnss and to the any client
    // connected to the socket.
    file_read_thread_ = std::thread([this]() { ReadNmeaFromLocalFile(); });
}

void GnssGrpcProxyServiceImpl::ReadNmeaFromLocalFile() {
    std::ifstream file(PathUtils::asUnicodePath(gnss_file_path_).c_str());
    if (file.is_open()) {
        std::string line;
        std::string lastLine;
        int count = 0;
        while (std::getline(file, line)) {
            count++;
            /* Only support a lite version of NEMA format to make it simple.
             * Records will only contains $GPGGA, $GPRMC,
             * $GPGGA,213204.00,3725.371240,N,12205.589239,W,7,,0.38,-26.75,M,0.0,M,0.0,0000*78
             * $GPRMC,213204.00,A,3725.371240,N,12205.589239,W,000.0,000.0,290819,,,A*49
             * $GPGGA,....
             * $GPRMC,....
             * Sending at 1Hz, currently user should
             * provide a NMEA file that has one location per second. need some
             * extra work to make it more generic, i.e. align with the timestamp
             * in the file.
             */
            if (count % 2 == 0) {
                {
                    std::lock_guard<std::mutex> lock(cached_nmea_mutex);
                    cached_nmea = lastLine + '\n' + line;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            } else {
            }
            lastLine = line;
        }
        file.close();
    } else {
        return;
    }
}

[[noreturn]] void GnssGrpcProxyServiceImpl::ReadLoop() {
    std::vector<char> buffer(GNSS_SERIAL_BUFFER_SIZE);
    int total_read = 0;
    std::string gnss_cmd_str;

    while (true) {
        auto bytes_read = gnss_out_->Read(buffer.data(), buffer.size());
        if (bytes_read > 0) {
            std::string s(buffer.data(), bytes_read);
            gnss_cmd_str += s;
            // In case random string sent though /dev/gnss0, gnss_cmd_str will
            // auto resize, to get rid of first page.
            if (gnss_cmd_str.size() > GNSS_SERIAL_BUFFER_SIZE * 2) {
                gnss_cmd_str = gnss_cmd_str.substr(gnss_cmd_str.size() -
                                                   GNSS_SERIAL_BUFFER_SIZE);
            }
            total_read += bytes_read;
            if (gnss_cmd_str.find(CMD_GET_LOCATION) != std::string::npos) {
                sendToSerial();
                gnss_cmd_str = "";
                total_read = 0;
            }
        } else {
            // just sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void GnssGrpcProxyServiceImpl::oneShotSendNmea(const char* data, int len) {
    std::string mystr;
    mystr.assign(data, len);
    mystr += "\n";
    std::lock_guard<std::mutex> lock(cached_nmea_mutex);
    legacy_nmea += mystr;
    if (legacy_nmea.find("GPGGA") != std::string::npos &&
        legacy_nmea.find("GPRMC") != std::string::npos) {
        cached_nmea = legacy_nmea;
        legacy_nmea.clear();
    }
}
}  // namespace cuttlefish

extern "C" {

void android_gnssgrpcv1_send_nmea(const char* data, int len) {
    if (!cuttlefish::s_proxy || !data || len <= 0)
        return;

    cuttlefish::s_proxy->oneShotSendNmea(data, len);
}

}  // external callable
