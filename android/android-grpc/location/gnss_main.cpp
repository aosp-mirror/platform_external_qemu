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

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "grpcpp.h"

#include "gnss_grpc_proxy.grpc.pb.h"
#include "gnss_grpc_proxy.h"

#include <common/libs/fs/shared_select.h>
#include <signal.h>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include "aemu/base/sockets/SocketUtils.h"

using cuttlefish::GnssGrpcProxyServiceImpl;

using cuttlefish::SharedFD;
struct MyThread {
    std::thread mT1;
    ~MyThread() {
        if (mT1.joinable()) {
            mT1.join();
        }
    }
};

static MyThread s_mythread;

void RunServer(SharedFD gnss_fd,
               std::string gnss_grpc_port,
               std::string gnss_file_path) {
    // need to listen at gnss_fd first, and accept a connect, and then pass that
    // to the serverimpl
    cuttlefish::SharedFDSet read_set;
    read_set.Set(gnss_fd);
    cuttlefish::Select(&read_set, nullptr, nullptr, nullptr);
    auto gnssguest = cuttlefish::SharedFD::Accept(*gnss_fd);
    GnssGrpcProxyServiceImpl service(gnssguest, gnssguest, gnss_file_path);
    service.StartServer();

    if (!gnss_file_path.empty()) {
        service.StartReadFileThread();
    }

    if (!gnss_grpc_port.empty()) {
        auto server_address("0.0.0.0:" + gnss_grpc_port);
        ServerBuilder builder;
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address,
                                 grpc::InsecureServerCredentials());
        // Register "service" as the instance through which we'll communicate
        // with clients. In this case it corresponds to an *synchronous*
        // service.
        builder.RegisterService(&service);
        // Finally assemble the server.
        std::unique_ptr<Server> server(builder.BuildAndStart());

        // Wait for the server to shutdown. Note that some other thread must be
        // responsible for shutting down the server for this call to ever
        // return.
        server->Wait();
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

extern "C" {
void android_gps_set_send_nmea_func(void* fptr);
void android_gnssgrpcv1_send_nmea(const char*, int);
}

int start_android_gnss_grpc_detached(bool& isIpv4,
                                     std::string grpcport,
                                     std::string gnssfilepath) {
    int request_port = 0;  // pick an available port
    auto shared_fd = cuttlefish::SharedFD::SocketLocalServer(request_port);
    isIpv4 = shared_fd.isIpv4();
    int actual_port = android::base::socketGetPort(shared_fd->getFd());
    std::thread t1(&RunServer, shared_fd, grpcport, gnssfilepath);
    t1.detach();
    s_mythread.mT1 = std::move(t1);

    android_gps_set_send_nmea_func((void*)android_gnssgrpcv1_send_nmea);

    return actual_port;
}
