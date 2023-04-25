
// Copyright (C) 2022 The Android Open Source Project
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

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/time/time.h"

using namespace std::chrono_literals;

ABSL_FLAG(absl::Duration,
          sleep,
          absl::Milliseconds(100),
          "Time to sleep before exiting");
ABSL_FLAG(int64_t, exit, 0, "The exit code to return");

ABSL_FLAG(std::string, msg_std_out, "", "Message to output on stdout");
ABSL_FLAG(std::string, msg_std_err, "", "Message to output on stderr");
int main(int argc, char** argv) {
    absl::ParseCommandLine(argc, argv);
    std::string msg = absl::GetFlag(FLAGS_msg_std_out);
    std::string err = absl::GetFlag(FLAGS_msg_std_err);

    if (!msg.empty()) {
        std::cout << msg;
        std::flush(std::cout);
    }

    if (!err.empty()) {
        std::cerr << err;
        std::flush(std::cerr);
    }

    std::chrono::milliseconds time_in_ms =
            absl::ToChronoMilliseconds(absl::GetFlag(FLAGS_sleep));
    std::this_thread::sleep_for(time_in_ms);
    return absl::GetFlag(FLAGS_exit);
}
