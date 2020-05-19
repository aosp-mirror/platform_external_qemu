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
#include <string>
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/globals.h"
#include "android/telephony/modem.h"
#include "android/utils/system.h"
// from cuttlefish modem-simulator library
#include "modem_simulator.h"

#define MODEM_DEBUG 0

#if MODEM_DEBUG
#define DD(fmt, ...)                                                        \
    fprintf(stderr, "modem-simulator: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define DD(fmt, ...)
#endif

    extern "C" {
int android_modem_version = 1;
    }

namespace cuttlefish {

void main_host_thread();

using ModemList = std::vector<std::shared_ptr<cuttlefish::ModemSimulator>>;
using FdList = std::vector<cuttlefish::SharedFD>;

static int s_host_server_port = -1;
static FdList modem_host_servers;
static constexpr int kCapacity = 300;

using MessageChannel = android::base::MessageChannel<ModemMessage, kCapacity>;

static MessageChannel s_msg_channel;
static bool s_stop_requested = false;

void queue_modem_message(ModemMessage msg) {
    s_msg_channel.send(msg);
}

void send_sms_msg(std::string msg) {
    ModemMessage mymsg;
    mymsg.type = MODEM_MSG_SMS;
    mymsg.sdata = msg;
    DD("sending message ");
    s_msg_channel.send(mymsg);
}

void receive_inbound_call(std::string number) {
    ModemMessage mymsg;
    mymsg.type = MODEM_MSG_CALL;
    std::string ss("REM0");
    ss += "AT+REMOTECALL=4,0,0,";
    ss += number;
    ss += ",129\r";  // not international
    mymsg.sdata = ss;
    DD("inbound call from %s", number.c_str());
    s_msg_channel.send(mymsg);
}

void disconnect_call(std::string number) {
    ModemMessage mymsg;
    mymsg.type = MODEM_MSG_ENDCALL;
    std::string ss("");
    ss += "AT+REMOTECALL=6,0,0,";
    ss += number;
    ss += ",129\r";  // not international
    mymsg.sdata = ss;
    DD("disconnect call from %s", number.c_str());
    s_msg_channel.send(mymsg);
}

void update_call(std::string number, int state) {
    ModemMessage mymsg;
    mymsg.type = MODEM_MSG_UPDATECALL;
    std::string ss("");
    ss += "AT+REMOTECALL=";
    ss += std::to_string(state);
    ss += ",0,0,";
    ss += number;
    ss += ",129\r";  // not international
    mymsg.sdata = ss;
    DD("update call from %s", number.c_str());
    s_msg_channel.send(mymsg);
}

enum ModemTechnology {
    M_MODEM_TECH_GSM = 1 << 0,
    M_MODEM_TECH_WCDMA = 1 << 1,
    M_MODEM_TECH_CDMA = 1 << 2,
    M_MODEM_TECH_EVDO = 1 << 3,
    M_MODEM_TECH_TDSCDMA = 1 << 4,
    M_MODEM_TECH_LTE = 1 << 5,
    M_MODEM_TECH_NR = 1 << 6,
};

void set_data_network_type(int type) {
    int ctec = M_MODEM_TECH_NR;
    if (type <= A_DATA_NETWORK_EDGE) {
        ctec = M_MODEM_TECH_GSM;
    } else if (type <= A_DATA_NETWORK_UMTS) {
        ctec = M_MODEM_TECH_WCDMA;
    } else if (type <= A_DATA_NETWORK_LTE) {
        ctec = M_MODEM_TECH_LTE;
    }

    ModemMessage mymsg;
    mymsg.type = MODEM_MSG_CTEC;
    std::string ss("AT+REMOTECTEC: ");
    ss += std::to_string(ctec);
    ss += "\r";
    mymsg.sdata = ss;
    s_msg_channel.send(mymsg);
}

void set_signal_strength_profile(int quality) {
    ModemMessage mymsg;
    mymsg.type = MODEM_MSG_SIGNAL;
    quality = 25 * quality;  // emualtor signal from 0 to 4, scale up to 100
    std::string ss("AT+REMOTESIGNAL: ");
    ss += std::to_string(quality);
    ss += "\r";
    mymsg.sdata = ss;
    s_msg_channel.send(mymsg);
}

void set_data_registration(int state) {
    ModemMessage mymsg;
    mymsg.type = MODEM_MSG_DATA_REG;
    std::string ss("AT+REMOTEREG: ");
    ss += std::to_string(state);
    ss += "\r";
    mymsg.sdata = ss;
    s_msg_channel.send(mymsg);
}

void set_voice_registration(int state) {
    ModemMessage mymsg;
    mymsg.type = MODEM_MSG_VOICE_REG;
    std::string ss("AT+REMOTEREG: ");
    ss += std::to_string(state);
    ss += "\r";
    mymsg.sdata = ss;
    s_msg_channel.send(mymsg);
}

static ModemList modem_simulators;
static ChannelMonitor* s_channel_monitor = nullptr;
static cuttlefish::SharedFD s_one_shot_monitor_sock;
static cuttlefish::SharedFD s_current_call_monitor_sock;

void start_calling_thread(std::string ss) {
    // connect and send message
    s_current_call_monitor_sock =
            cuttlefish::SharedFD::SocketLocalClient(s_host_server_port);
    if (s_current_call_monitor_sock->IsOpen()) {
        DD("making a connection to main host server %d", s_host_server_port);
        s_current_call_monitor_sock->Write(ss.data(), ss.size());
        std::string ss;
        while (true) {
            std::vector<char> buffer(1024);
            auto bytes_read = s_current_call_monitor_sock->Read(buffer.data(),
                                                                buffer.size());
            if (bytes_read <= 0) {
                DD("nothing read quit");
                break;
            } else {
                ss.append(buffer.data());
                if (ss == "KO") {
                    DD("got: %s", ss.c_str());
                    break;
                } else if (ss == "OK") {
                    DD("got: %s", ss.c_str());
                    break;
                }
            }
        }
    } else {
        DD("ERROR: cannot making a connection to main host server %d",
           s_host_server_port);
    }
    s_current_call_monitor_sock = cuttlefish::SharedFD();
    DD("done with this thread");
}

void process_msgs() {
    while (true) {
        if (s_stop_requested) {
            break;
        }
        if (!guest_boot_completed) {
            sleep_ms(100);
            continue;
        }
        DD("waiting for new messages ...");
        ModemMessage msg;
        s_msg_channel.receive(&msg);
        if (msg.type == MODEM_MSG_QUIT) {
            DD("quit now");
            break;
        }
        if (msg.type == MODEM_MSG_SMS) {
            s_channel_monitor->SendUnsolicitedCommand(msg.sdata);
        } else if (msg.type == MODEM_MSG_CALL) {
            if (!s_current_call_monitor_sock->IsOpen()) {
                std::thread t1(&start_calling_thread, msg.sdata);
                t1.detach();
            }
        } else if (msg.type == MODEM_MSG_ENDCALL ||
                   msg.type == MODEM_MSG_UPDATECALL) {
            if (s_current_call_monitor_sock->IsOpen()) {
                s_current_call_monitor_sock->Write(msg.sdata.data(),
                                                   msg.sdata.size());
            }
        } else if (msg.type == MODEM_MSG_CTEC || msg.type == MODEM_MSG_SIGNAL ||
                   msg.type == MODEM_MSG_DATA_REG ||
                   msg.type == MODEM_MSG_VOICE_REG) {
            if (!s_one_shot_monitor_sock->IsOpen()) {
                s_one_shot_monitor_sock =
                        cuttlefish::SharedFD::SocketLocalClient(
                                s_host_server_port);
                if (s_one_shot_monitor_sock->IsOpen()) {
                    std::string ss("REM0\r");
                    s_one_shot_monitor_sock->Write(ss.data(), ss.size());
                } else {
                    DD("sending to main host server");
                }
            }
            if (s_one_shot_monitor_sock->IsOpen()) {
                s_one_shot_monitor_sock->Write(msg.sdata.data(),
                                               msg.sdata.size());
            }
        }
    }
}

FdList ServerFdsFromCmdline(const std::string& modem_port_string) {
    // Validate the parameter
    std::vector<cuttlefish::SharedFD> shared_fds;
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
        auto shared_fd = cuttlefish::SharedFD::SocketLocalServer(fd);
        shared_fds.push_back(shared_fd);
    }

    return shared_fds;
}


static int start_android_modem_host_server(std::string server_port) {
    int request_port = std::stoi(server_port);
    auto shared_fd = cuttlefish::SharedFD::SocketLocalServer(request_port);
    modem_host_servers.push_back(shared_fd);
    int actual_port = android::base::socketGetPort(shared_fd->getFd());
    return actual_port;
}

static FdList modem_guest_servers;

static int start_android_modem_guest_server() {
    int request_port = 0; // pick an available port
    auto shared_fd = cuttlefish::SharedFD::SocketLocalServer(request_port);
    modem_guest_servers.push_back(shared_fd);
    int actual_port = android::base::socketGetPort(shared_fd->getFd());
    return actual_port;
}

int stop_android_modem_simulator() {
    if (s_host_server_port > 0 && !modem_host_servers.empty()) {
        auto nvram_config = cuttlefish::NvramConfig::Get();
        auto nvram_config_file = nvram_config->ConfigFileLocation();
        bool success = nvram_config->SaveToFile(nvram_config_file);
        if (success == false) {
            DD("failed to save nvram json file %s", nvram_config_file.c_str());
        } else {
            DD("successfully saved nvram json file %s",
               nvram_config_file.c_str());
        }
        for (auto modem : modem_simulators) {
            DD("save modem now");
            modem->SaveModemState();
        }
        // send STOP to it
        auto monitor_sock =
                cuttlefish::SharedFD::SocketLocalClient(s_host_server_port);
        std::string msg("STOP");
        if (monitor_sock->IsOpen()) {
            LOG(VERBOSE) << "sending STOP to modem simulator host server";
            monitor_sock->Write(msg.data(), msg.size());
            return 0;
        } else {
            DD("cannot connect to host server at port %d", s_host_server_port);
        }
    }
    return -1;
}

struct MyThread {
    std::thread mT1;
    std::thread mT2;
    ~MyThread() {
        ModemMessage msg;
        s_stop_requested = true;
        msg.type = MODEM_MSG_QUIT;
        queue_modem_message(msg);
        if (mT1.joinable()) {
            mT1.join();
        }

        if (s_host_server_port > 0 && !modem_host_servers.empty()) {
            // send STOP to it
            auto monitor_sock =
                    cuttlefish::SharedFD::SocketLocalClient(s_host_server_port);
            std::string msg("STOP");
            if (monitor_sock->IsOpen()) {
                DD("sending STOP to main host server");
                monitor_sock->Write(msg.data(), msg.size());
                if (mT2.joinable()) {
                    mT2.join();
                }
            } else {
                DD("cannot connect to host server at port %d",
                   s_host_server_port);
            }
        } else {
            if (mT2.joinable()) {
                mT2.join();
            }
        }
    }
};

static MyThread s_mythread;

// start with a given guest fd, and host fd
int start_android_modem_simulator_detached() {
    android_modem_version = 2;

    int actual_guest_server_port = start_android_modem_guest_server();

    {
        int32_t modem_id = 0;

        auto& server_fds = modem_guest_servers;
        constexpr int sim_type = 1;
        cuttlefish::NvramConfig::InitNvramConfigService(server_fds.size(), sim_type);

        for (auto& fd : server_fds) {
            auto modem_simulator =
                    std::make_shared<cuttlefish::ModemSimulator>(modem_id);
            auto channel_monitor = std::make_unique<cuttlefish::ChannelMonitor>(
                    modem_simulator.get(), fd);

            if (s_channel_monitor == nullptr) {
                s_channel_monitor = channel_monitor.get();
            }
            modem_simulator->Initialize(std::move(channel_monitor));

            modem_simulators.push_back(modem_simulator);

            modem_id++;
        }
    }

    std::thread t1(&process_msgs);
    s_mythread.mT1 = std::move(t1);

    s_host_server_port = start_android_modem_host_server("0");
    std::thread t2(&main_host_thread);
    s_mythread.mT2 = std::move(t2);

    DD("starring modem simulator ready at guest port %d host port %d",
       actual_guest_server_port, s_host_server_port);
    return actual_guest_server_port;
}

void main_host_thread() {
    if (modem_host_servers.empty()) {
        return;
    }

    auto monitor_socket = modem_host_servers[0];
    LOG(VERBOSE) << "started modem simulator host server at port: "
                 << s_host_server_port;
    while (true) {
        DD("looping at main host server at %d", s_host_server_port);
        cuttlefish::SharedFDSet read_set;
        read_set.Set(monitor_socket);
        int num_fds = cuttlefish::Select(&read_set, nullptr, nullptr, nullptr);
        if (num_fds <= 0) {  // Ignore select error
            DD("Select call returned error %s ", strerror(errno));
        } else if (read_set.IsSet(monitor_socket)) {
            DD("got connection at host server at %d", s_host_server_port);
            auto conn = cuttlefish::SharedFD::Accept(*monitor_socket);
            std::string buf(4, ' ');
            auto read = cuttlefish::ReadExact(conn, &buf);
            if (read <= 0) {
                conn->Close();
                DD("Detected close from the other side");
                continue;
            }
            if (buf == "STOP") {  // Exit request from parent process
                LOG(VERBOSE) << "received exit request from parent process";
                break;
            } else if (buf.compare(0, 3, "REM") ==
                       0) {  // REMO for modem id 0 ...
                // Remote request from other cuttlefish instance
                DD("got a call");
                int id = std::stoi(buf.substr(3, 1));
                if (id >= modem_simulators.size()) {
                    DD("Not supported modem simulator count: %d", id);
                } else {
                    DD("pass to channel monitior %d", id);
                    modem_simulators[id]->SetRemoteClient(conn, true);
                }
            } else {
                DD("not sure I understand %s", buf.c_str());
            }
        }
    }

    modem_host_servers.clear();
    modem_simulators.clear();
}

}  // namespace cuttlefish
