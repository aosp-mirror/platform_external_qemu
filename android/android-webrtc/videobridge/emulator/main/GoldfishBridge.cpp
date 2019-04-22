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
#include "emulator/main/flagdefs.h"
#include "rtc_base/logging.h"
#include "rtc_base/ssladapter.h"
#include "emulator/net/EmulatorConnection.h"

using emulator::webrtc::Switchboard;

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
    std::unique_ptr<StdLogSink> log_sink = nullptr;

    if (FLAG_help) {
        rtc::FlagList::Print(NULL, false);
        return 0;
    }

    if (FLAG_verbose) {
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

    rtc::InitializeSSL();
    emulator::net::EmulatorConnection server(FLAG_port, FLAG_handle);
    server.listen(); // Fork, so we can guarantee caller that port is available?
    rtc::CleanupSSL();
    return 0;
}
