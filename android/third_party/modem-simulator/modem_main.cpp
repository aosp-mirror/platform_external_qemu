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

// main method for stand-alone modem simulator
// note, it could simulator multiple modems on a single phone,
// but for simplicity, just assume one modem here

#include "modem_main.h"
#include <gflags/gflags.h>
#include <string>
#include "modem_simulator.h"

#define MODEM_DEBUG 1

#if MODEM_DEBUG
#define DD(fmt, ...)                                                        \
    fprintf(stderr, "modem-simulator: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define DD(fmt, ...)
#endif

DEFINE_bool(standalone, false, "modem simulator runs a standalone binary");
DEFINE_int32(instance_number, 1, "modem simulator instance numbers");
DEFINE_string(modem_port,
              "",
              "the port that guest ril will send AT commands to");
DEFINE_string(phone_number,
              "",
              "phone number of this simulator(4 digits port number) that other "
              "phone could talk to");

namespace cvd {
std::vector<cvd::SharedFD> ServerFdsFromCmdline(
        const std::string& modem_port_string) {
    // Validate the parameter
    std::vector<cvd::SharedFD> shared_fds;
    std::string fd_list = modem_port_string;
    for (auto c : fd_list) {
        if (c != ',' && (c < '0' || c > '9')) {
            LOG(ERROR) << "Invalid file descriptor list: " << fd_list;
            return shared_fds;
        }
    }

    auto fds = android::base::Split(fd_list, ",");
    for (auto& fd_str : fds) {
        auto fd = std::stoi(fd_str);
        auto shared_fd = cvd::SharedFD::SocketLocalServer(fd);
        shared_fds.push_back(shared_fd);
    }

    return shared_fds;
}

static bool s_should_quit = false;

void start_android_modem_simulator(const std::string& modem_port_string,
                                   const std::string& phone_number_string) {
    auto server_fds = ServerFdsFromCmdline(modem_port_string);

    if (server_fds.empty()) {
        return;
    }

    auto nvram_config = cvd::NvramConfig::Get();
    auto nvram_config_file = nvram_config->ConfigFileLocation();

    // Start channel monitor, wait for RIL to connect
    int32_t modem_id = 0;
    std::vector<std::shared_ptr<cvd::ModemSimulator>> modem_simulators;
    std::vector<std::shared_ptr<cvd::ChannelMonitor>> channel_monitors;

    for (auto& fd : server_fds) {
        auto modem_simulator = std::make_shared<cvd::ModemSimulator>(modem_id);
        auto channel_monitor =
                std::make_shared<cvd::ChannelMonitor>(modem_simulator, fd);

        modem_simulator->Initialize(channel_monitor);

        modem_simulators.push_back(modem_simulator);
        channel_monitors.push_back(channel_monitor);

        modem_id++;
    }

    int phone_number = atoi(phone_number_string.c_str());

    auto monitor_socket = cvd::SharedFD::SocketLocalServer(phone_number);
    if (!monitor_socket->IsOpen()) {
        DD("Unable to create monitor socket for modem simulator");
        std::exit(cvd::kServerError);
    }

    timeval my10Mstimeout{0 /* 0 sec*/, 10000 /* 10*000 usec*/};

    // Server loop
    while (true) {
        cvd::SharedFDSet read_set;
        read_set.Set(monitor_socket);
        int num_fds = cvd::Select(&read_set, nullptr, nullptr, &my10Mstimeout);
        if (s_should_quit) {
            DD("requested to quit now");
            break;
        }
        if (num_fds <= 0) {  // Ignore select error
            // DD("Select call returned error %s ", strerror(errno));
        } else if (read_set.IsSet(monitor_socket)) {
            auto conn = cvd::SharedFD::Accept(*monitor_socket);
            std::string buf(4, ' ');
            auto read = cvd::ReadExact(conn, &buf);
            if (read <= 0) {
                conn->Close();
                DD("Detected close from the other side");
                continue;
            }
            if (buf == "STOP") {  // Exit request from parent process
                DD("Exit request from parent process");
                nvram_config->SaveToFile(nvram_config_file);
                for (auto modem : modem_simulators) {
                    modem->SaveModemState();
                }
                cvd::WriteAll(conn,
                              "OK");  // Ignore the return value. Exit anyway.
                break;
            } else if (buf.compare(0, 3, "REM") ==
                       0) {  // REMO for modem id 0 ...
                // Remote request from other cuttlefish instance
                int id = std::stoi(buf.substr(3, 1));
                if (id >= channel_monitors.size()) {
                    DD("Not supported modem simulator count: %d", id);
                } else {
                    channel_monitors[id]->SetRemoteClient(conn, true);
                }
            }
        }
    }
}

void start_android_modem_simulator_detached(
        const std::string& modem_port_string) {
    DD("starring modem simulator ...");
    // start a thread
    std::string phone_number_string(
            std::to_string(std::atoi(modem_port_string.c_str()) + 1));
    std::thread t1(&start_android_modem_simulator, modem_port_string,
                   phone_number_string);
    t1.detach();
    DD("starring modem simulator ready ");
}

}  // namespace cvd

extern "C" {
void request_android_modem_simulator_stop() {
    cvd::s_should_quit = true;
}
}
