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
#include <rtc_base/flags.h>                    // for FlagList
#include <stdio.h>                             // for fprintf, printf, NULL
#include <memory>                              // for unique_ptr
#include <string>                              // for string, operator!=

#include "android/base/StringView.h"           // for StringView
#include "android/base/memory/SharedMemory.h"  // for SharedMemory, SharedMe...
#include "emulator/main/flagdefs.h"            // for FLAG_handle, FLAG_port
#include "emulator/net/EmulatorConnection.h"   // for EmulatorConnection
#include "rtc_base/logging.h"                  // for LogSink, RTC_LOG, INFO
#include "rtc_base/logsinks.h"                 // for FileRotatingLogSink
#include "rtc_base/ssladapter.h"               // for CleanupSSL, InitializeSSL

namespace emulator {
namespace webrtc {
class Switchboard;
}  // namespace webrtc
}  // namespace emulator

using android::base::SharedMemory;
using emulator::webrtc::Switchboard;

const int kMaxFileLogSizeInBytes = 64 * 1024 * 1024;

// A simple logsink that throws everyhting on stderr.
class StdLogSink : public rtc::LogSink {
public:
    StdLogSink() = default;
    ~StdLogSink() override {}

    void OnLogMessage(const std::string& message) override {
        fprintf(stderr, "%s", message.c_str());
    }
};

int main(int argc, char* argv[]) {
    rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, true);
    std::unique_ptr<rtc::LogSink> log_sink = nullptr;

    if (FLAG_help) {
        rtc::FlagList::Print(NULL, false);
        return 0;
    }

    // Configure our loggers, we will log at most 1
    if (FLAG_logdir != std::string("")) {
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
    emulator::net::EmulatorConnection server(FLAG_port, FLAG_handle, FLAG_turn);
    int status = server.listen(deamon) ? 0 : 1;
    RTC_LOG(INFO) << "Finished, status: " << status;
    rtc::CleanupSSL();
    return status;
}
