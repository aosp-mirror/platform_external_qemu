//
// Copyright 2017 The Android Open Source Project
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
//
#include "net/posix/posix_async_socket_connector.h"
#include "net/posix/posix_async_socket_server.h"
#include "test_environment.h"

#include <future>

#include "os/log.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "model/setup/async_manager.h"

using ::android::bluetooth::root_canal::TestEnvironment;
using ::android::net::PosixAsyncSocketConnector;
using ::android::net::PosixAsyncSocketServer;
using test_vendor_lib::AsyncManager;

ABSL_FLAG(std::string,
          controller_properties_file,
          "",
          "controller_properties.json file path");
ABSL_FLAG(std::string,
          default_commands_file,
          "",
          "commands file which root-canal runs it as default");

constexpr uint16_t kTestPort = 6401;
constexpr uint16_t kHciServerPort = 6402;
constexpr uint16_t kLinkServerPort = 6403;

extern "C" const char* __asan_default_options() {
    return "detect_container_overflow=0";
}

int main(int argc, char** argv) {
    auto leftovers = absl::ParseCommandLine(argc, argv);

    LOG_INFO("main");
    uint16_t test_port = kTestPort;
    uint16_t hci_server_port = kHciServerPort;
    uint16_t link_server_port = kLinkServerPort;

    int idx = 0;
    for (auto arg : leftovers) {
        idx++;
        int port = atoi(arg);
        LOG_INFO("%d: %s (%d)", idx, arg, port);
        if (port < 1 || port > 0xffff) {
            LOG_WARN("%s out of range", arg);
        } else {
            switch (idx) {
                case 1:
                    test_port = port;
                    break;
                case 2:
                    hci_server_port = port;
                    break;
                case 3:
                    link_server_port = port;
                    break;
                default:
                    LOG_WARN("Ignored option %s", arg);
            }
        }
    }

    AsyncManager am;
    TestEnvironment root_canal(
            std::make_shared<PosixAsyncSocketServer>(test_port, &am),
            std::make_shared<PosixAsyncSocketServer>(hci_server_port, &am),
            std::make_shared<PosixAsyncSocketServer>(link_server_port, &am),
            std::make_shared<PosixAsyncSocketConnector>(&am),
            absl::GetFlag(FLAGS_controller_properties_file),
            absl::GetFlag(FLAGS_default_commands_file));
    std::promise<void> barrier;
    std::future<void> barrier_future = barrier.get_future();
    root_canal.initialize(std::move(barrier));
    barrier_future.wait();
    root_canal.close();
}
