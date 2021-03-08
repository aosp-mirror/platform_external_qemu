// Copyright 2020 The Android Open Source Project
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

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace android {
namespace emulation {

class PassiveGpsUpdater {
public:
    PassiveGpsUpdater(std::function<void(void)> func);
    ~PassiveGpsUpdater();

public:
    // parse the location data from AVDconf; changes nothing if AVDconf
    // not readable
    static void parseLocationConf(const std::string& AVDconf,
                                  double& platitude,
                                  double& plongitude,
                                  double& paltitude,
                                  double& pvelocity,
                                  double& pheading);

private:
    void start();
    void stop();

private:
    std::condition_variable mCv;
    std::mutex mMutex;
    std::thread mThread;
    bool mStopRequested{false};

    std::function<void(void)> mFunction;
};

}  // namespace emulation
}  // namespace android
