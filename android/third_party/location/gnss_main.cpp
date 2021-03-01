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




void RunServer() {
    while(true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

int location_main(int argc, char** argv) {
  RunServer();

  return 0;
}
