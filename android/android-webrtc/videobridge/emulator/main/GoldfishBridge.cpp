// Copyright (C) 2019 The Android Open Source Project
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
#include <rtc_base/log_sinks.h>                      // for FileRotatingLogSink
#include <rtc_base/logging.h>                        // for RTC_LOG, LogSink
#include <rtc_base/ssl_adapter.h>                    // for CleanupSSL, Init...
#include <stdio.h>                                   // for fprintf, printf
#include <stdlib.h>                                  // for atoi, exit, EXIT...
#include <unistd.h>                                  // for fork, pid_t
#include <memory>                                    // for unique_ptr
#include <string>                                    // for basic_string
#include <vector>                                    // for vector

#include "android/base/Optional.h"                   // for Optional
#include "android/base/ProcessControl.h"             // for parseEscapedLaun...
#include "android/base/system/System.h"              // for System, System::...
#include "android/emulation/control/GrpcServices.h"  // for EmulatorControll...
#include "android/emulation/control/RtcService.h"    // for getRtcService
#include "emulator/net/EmulatorGrcpClient.h"         // for EmulatorGrpcClient
#include "emulator/webrtc/Switchboard.h"             // for Switchboard
#include "nlohmann/json.hpp"                         // for json

namespace android {
namespace base {
class SharedMemory;
}  // namespace base
}  // namespace android

#ifdef _MSC_VER
#include "msvc-getopt.h"
#include "msvc-posix.h"
#else
#include <getopt.h>                                  // for optarg, required...
#endif

using nlohmann::json;

static std::string FLAG_logdir("");
static std::string FLAG_server("localhost:8554");
static std::string FLAG_turn("");
static bool FLAG_help = false;
static bool FLAG_verbose = true;
static int FLAG_port = 8555;
static std::string FLAG_disc("");
static bool FLAG_daemon = false;
static std::string FLAG_shm("");

static struct option long_options[] = {{"logdir", required_argument, 0, 'l'},
                                       {"server", required_argument, 0, 's'},
                                       {"turn", required_argument, 0, 't'},
                                       {"help", no_argument, 0, 'h'},
                                       {"verbose", no_argument, 0, 'v'},
                                       {"port", required_argument, 0, 'p'},
                                       {"daemon", no_argument, 0, 'd'},
                                       {"discovery", required_argument, 0, 'i'},
                                       {"shm_path", required_argument, 0, 'm'},
                                       {0, 0, 0, 0}};

using android::emulation::control::EmulatorControllerService;
using emulator::webrtc::EmulatorGrpcClient;
using emulator::webrtc::Switchboard;

static void printUsage() {
    printf("--logdir (Directory to log files to, or empty when unused)  type: "
           "string  default:\n"
           "--turn (Process that will be invoked to retrieve TURN json "
           "configuration.)  type: string  default:\n"
           "--daemon (Run as a deamon, will print PID of deamon upon exit)  "
           "type: bool  default: false\n"
           "--verbose (Enables logging to stdout)  type: bool  default: false\n"
           "--port (Port to launch gRPC service on)  type: int  default: 8555\n"
           "--disc (The discoveryfile.) Use discovery file instead of server flag type: string\n"
           "--server (The emulator grpc server to connect to.)  type: string  default: "
           "localhost:8554\n"
           "--help (Prints this message)  type: bool  default: false\n");
}

static void parseArgs(int argc, char** argv) {
    int long_index = 0;
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) !=
           -1) {
        switch (opt) {
            case 'l':
                FLAG_logdir = optarg;
                break;
            case 'm':
                FLAG_shm = optarg;
            case 's':
                FLAG_server = optarg;
                break;
            case 't':
                FLAG_turn = optarg;
                break;
            case 'v':
                FLAG_verbose = true;
                break;
            case 'p':
                FLAG_port = atoi(optarg);
                break;
            case 'i':
                FLAG_disc = optarg;
                break;
            case 'd':
                FLAG_daemon = true;
                break;
            case 'h':
            default:
                printUsage();
                exit(EXIT_FAILURE);
        }
    }
}

const int kMaxFileLogSizeInBytes = 128 * 1024;

// A simple logsink that throws everyhting on stderr.
class StdLogSink : public rtc::LogSink {
public:
    StdLogSink() = default;
    ~StdLogSink() override {}

    void OnLogMessage(const std::string& message) override {
        fprintf(stderr, "Log: %s", message.c_str());
    }
};


int main(int argc, char* argv[]) {
    parseArgs(argc, argv);

    std::unique_ptr<rtc::LogSink> log_sink = nullptr;

    // Configure our loggers, we will log at most 1
    if (FLAG_logdir != "") {
        rtc::FileRotatingLogSink* frl = new rtc::FileRotatingLogSink(
                FLAG_logdir, "goldfish_rtc", kMaxFileLogSizeInBytes, 2);
        frl->Init();
        frl->DisableBuffering();
        log_sink.reset(frl);
        rtc::LogMessage::AddLogToStream(log_sink.get(),
                                        rtc::LoggingSeverity::INFO);
    } else if (FLAG_verbose && !FLAG_daemon) {
        log_sink.reset(new StdLogSink());
        rtc::LogMessage::AddLogToStream(log_sink.get(),
                                        rtc::LoggingSeverity::INFO);
    }

    // Abort if the user specifies a port that is outside the allowed
    // range [1, 65535].
    if ((FLAG_port < 1) || (FLAG_port > 65535)) {
        printf("Error: %i is not a valid port.\n", FLAG_port);
        return -1;
    }

    // Check to see if we have a working turn configuration.
    if (!FLAG_turn.empty()) {
        auto cmd = android::base::parseEscapedLaunchString(FLAG_turn);

        RTC_LOG(INFO) << "TurnCFG: --- Command --- ";
        for (auto param : cmd) {
            RTC_LOG(INFO) << "TurnCFG: param:" << param;
        }
        RTC_LOG(INFO) << "TurnCFG: --- Running --- ";

        android::base::System::ProcessExitCode exitCode;
        auto turn = android::base::System::get()->runCommandWithResult(
                cmd, 1000, &exitCode);

        if (turn) {
            RTC_LOG(INFO) << "TurnCFG:" << *turn;
            if (exitCode == 0 && json::accept(*turn)) {
                json config = json::parse(*turn, nullptr, false);
                if (config.count("iceServers")) {
                    RTC_LOG(INFO)
                            << "TurnCFG: Turn command produces valid json.";
                } else {
                    RTC_LOG(LS_ERROR) << "TurnCFG: JSON is missing iceServers.";
                }
            } else {
                RTC_LOG(INFO) << "TurnCFG: bad exit: " << exitCode
                              << ", or bad json result: " << *turn;
            }
        } else {
            RTC_LOG(INFO) << "TurnCFG: Produces no result, exit: " << exitCode;
        }
    }


    rtc::InitializeSSL();
    std::unique_ptr<EmulatorGrpcClient> client;

    if (!FLAG_disc.empty()) {
        client = std::make_unique<EmulatorGrpcClient>(FLAG_disc);
    } else {
        client = std::make_unique<EmulatorGrpcClient>(FLAG_server, "", "", "");
    }

    if (!client->hasOpenChannel()) {
        RTC_LOG(LERROR) << "Unable to open gRPC channel to the emulator.";
        return -1;
    }

    Switchboard sw(*client, FLAG_shm, FLAG_turn);
    auto bridge = android::emulation::control::getRtcService(&sw);

    // Local only?
    std::string address = "0.0.0.0";
    auto builder = EmulatorControllerService::Builder()
                           .withPortRange(FLAG_port, FLAG_port + 1)
                           .withVerboseLogging(FLAG_verbose)
                           .withAddress(address)
                           .withService(bridge);

    auto service = builder.build();


#ifndef _WIN32
    if (FLAG_daemon) {
        pid_t pid = ::fork();
        if (pid != 0) {
            RTC_LOG(INFO) << "Spawned a child under: " << pid;
            fprintf(stderr, "%d\n", pid);
            return 0;
        }
    }
#endif

    service->wait();
    rtc::CleanupSSL();
    return 0;
}
