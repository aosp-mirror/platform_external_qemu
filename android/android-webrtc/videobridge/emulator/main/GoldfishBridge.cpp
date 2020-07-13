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
#include <rtc_base/log_sinks.h>    // for FileRotatingLogSink
#include <rtc_base/logging.h>      // for LogSink, RTC_LOG, INFO
#include <rtc_base/ssl_adapter.h>  // for CleanupSSL, InitializeSSL
#include <stdio.h>                 // for printf, fprintf, stderr
#include <stdlib.h>                // for atoi, exit, EXIT_FAILURE

#include <memory>  // for unique_ptr
#include <string>  // for string, operator!=

#include "android/base/StringView.h"           // for StringView
#include "android/base/memory/SharedMemory.h"  // for SharedMemory, SharedMe...
#include "emulator/net/EmulatorConnection.h"   // for EmulatorConnection

#ifdef _MSC_VER
#include "msvc-posix.h"
#include "msvc-getopt.h"
#else
#include <getopt.h>
#endif

static std::string FLAG_logdir("");
static std::string FLAG_server("127.0.0.1");
static std::string FLAG_turn("");
static bool FLAG_help = false;
static bool FLAG_verbose = true;
static std::string FLAG_handle("video0");
static int FLAG_port = 5557;
static std::string FLAG_disc("");
static bool FLAG_daemon = false;

static struct option long_options[] = {{"logdir", required_argument, 0, 'l'},
                                       {"server", required_argument, 0, 's'},
                                       {"turn", required_argument, 0, 't'},
                                       {"handle", required_argument, 0, 'e'},
                                       {"help", no_argument, 0, 'h'},
                                       {"verbose", no_argument, 0, 'v'},
                                       {"port", required_argument, 0, 'p'},
                                       {"daemon", no_argument, 0, 'd'},
                                       {"discovery", required_argument, 0, 'i'},
                                       {0, 0, 0, 0}};

using android::base::SharedMemory;

static void printUsage() {
    printf("--logdir (Directory to log files to, or empty when unused)  type: "
           "string  default:\n"
           "--turn (Process that will be invoked to retrieve TURN json "
           "configuration.)  type: string  default:\n"
           "--daemon (Run as a deamon, will print PID of deamon upon exit)  "
           "type: bool  default: false\n"
           "--verbose (Enables logging to stdout)  type: bool  default: false\n"
           "--handle (The memory handle to read frames from)  type: string  "
           "default: video0\n"
           "--port (The port to connect to.)  type: int  default: 5557\n"
           "--disc (The discoveryfile.) type: string, required\n"
           "--server (The server to connect to.)  type: string  default: "
           "127.0.0.1\n"
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
            case 'e':
                FLAG_handle = optarg;
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

const int kMaxFileLogSizeInBytes = 64 * 1024 * 1024;

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

    // Check if we can access the shared memory region.
    {
        auto sharedMemory = SharedMemory(FLAG_handle, 4);
        int err = sharedMemory.open(SharedMemory::AccessMode::READ_ONLY);
        if (err != 0) {
            RTC_LOG(LERROR) << "Unable to open memory mapped handle: ["
                            << FLAG_handle << "], due to:" << -err;
            return -1;
        }
        RTC_LOG(INFO) << "Able to map first 4 bytes of: " << FLAG_handle;
    }

    rtc::InitializeSSL();
    bool deamon = FLAG_daemon;
    emulator::net::EmulatorConnection server(FLAG_port, FLAG_disc, FLAG_handle, FLAG_turn);
    int status = server.listen(deamon) ? 0 : 1;
    RTC_LOG(INFO) << "Finished, status: " << status;
    rtc::CleanupSSL();
    return status;
}
