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

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <grpcpp.h>

#include "gnss_grpc_proxy.h"
#include "gnss_grpc_proxy.grpc.pb.h"

#include <signal.h>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

using cuttlefish::GnssGrpcProxyServiceImpl;

using cuttlefish::SharedFD;

void RunServer() {
    std::string FLAGS_gnss_grpc_port = "9999";
    std::string FLAGS_gnss_file_path = "";

    // TODO: fix the following two fds
    SharedFD gnss_in, gnss_out;
  auto server_address("0.0.0.0:" + FLAGS_gnss_grpc_port);
  GnssGrpcProxyServiceImpl service(gnss_in, gnss_out, FLAGS_gnss_file_path);
  service.StartServer();
  if (!FLAGS_gnss_file_path.empty()) {
    service.StartReadFileThread();
    // In the local mode, we are not start a grpc server, use a infinite loop instead
    while(true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
  } else {
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
  }

}

int location_main(int argc, char** argv) {
  RunServer();

  return 0;
}
