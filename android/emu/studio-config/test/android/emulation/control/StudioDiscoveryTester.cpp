//
// Copyright (C) 2020 The Android Open Source Project
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
#include <cstdio>
#include <memory>
#include <ratio>
#include <string>
#include <thread>
#include <utility>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/time/time.h"
#include "aemu/base/Log.h"
#include "android/emulation/control/EmulatorAdvertisement.h"
#include "android/utils/path.h"

using android::emulation::control::EmulatorAdvertisement;
using android::emulation::control::EmulatorProperties;

ABSL_FLAG(absl::Duration,
          sleep,
          absl::Milliseconds(100),
          "Time to sleep before exiting");
ABSL_FLAG(std::string, discovery, "", "Path of discovery file");

int main(int argc, char* argv[]) {
    absl::ParseCommandLine(argc, argv);

    EmulatorProperties cfg({{"hello", "world"}});
    std::unique_ptr<EmulatorAdvertisement> discovery;
    std::string path = absl::GetFlag(FLAGS_discovery);

    if (path.empty()) {
        discovery = std::make_unique<EmulatorAdvertisement>(std::move(cfg));
        auto status = discovery->write();
        LOG(INFO) << (status ? "Successfully Wrote: " : "Failed to write: ")
                  << discovery->location();
    } else {
        discovery =
                std::make_unique<EmulatorAdvertisement>(std::move(cfg), path);
        auto status = discovery->write();
        LOG(INFO) << (status ? "Successfully Wrote: " : "Failed to write: ")
                  << discovery->location();
    }

    std::chrono::milliseconds time_in_ms =
            absl::ToChronoMilliseconds(absl::GetFlag(FLAGS_sleep));
    std::this_thread::sleep_for(time_in_ms);
    return 0;
}
