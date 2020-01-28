
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

#include <chrono>   // for microse...
#include <cstdio>   // for fprintf
#include <ratio>    // for ratio
#include <string>   // for basic_s...
#include <utility>  // for move

#include "android/base/Log.h"
#include "android/utils/path.h"
#include "android/base/system/System.h"                       // for System
#include "android/emulation/control/utils/StudioDiscovery.h"  // for StudioD...

using android::base::System;
using android::emulation::control::StudioConfig;
using android::emulation::control::StudioDiscovery;

int main(int argc, char* argv[]) {
    StudioConfig cfg({{"hello", "world"}});
    if (argc < 2) {
        StudioDiscovery discovery(std::move(cfg));
        discovery.write();
        LOG(INFO) << "Wrote: " << discovery.sharedFile();
    } else {
        auto path = argv[1];
        StudioDiscovery discovery(std::move(cfg), path);
        discovery.write();
        LOG(INFO) << "Wrote: " << discovery.sharedFile();
    }

    std::chrono::seconds three_seconds(3);
    System::get()->sleepMs(std::chrono::milliseconds(three_seconds).count());
    return 0;
}
