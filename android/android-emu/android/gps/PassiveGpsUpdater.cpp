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

#include "android/gps/PassiveGpsUpdater.h"

#include "android/base/files/PathUtils.h"      // for PathUtils

#include <fstream>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

using namespace std::chrono_literals;

using android::base::PathUtils;
namespace android {
namespace emulation {

PassiveGpsUpdater::PassiveGpsUpdater(std::function<void(void)> func)
    : mFunction(func) {
    std::thread t1([this] { start(); });
    mThread = std::move(t1);
}

void PassiveGpsUpdater::start() {
    while (true) {
        // wait for mStopRequested to be true or timeout 1sec
        {
            std::unique_lock<std::mutex> lk(mMutex);
            if (mCv.wait_for(lk, 1000ms, [this] { return mStopRequested; })) {
                if (mStopRequested)
                    break;
            }
        }
        // send gps fix to guest
        mFunction();
    }
}

void PassiveGpsUpdater::parseLocationConf(const std::string& AVDconf,
                                          double& platitude,
                                          double& plongitude,
                                          double& paltitude,
                                          double& pvelocity,
                                          double& pheading) {
    std::ifstream inFile(PathUtils::asUnicodePath(AVDconf).c_str());
    if (!inFile)
        return;

    std::string line;
    while (std::getline(inFile, line)) {
        // find the loc section
        if (line.find("loc\\") == 0) {
            auto pstart = line.find("=", 4);
            if (pstart != std::string::npos) {
                std::string key = line.substr(4, pstart - 4);
                std::string val = line.substr(pstart + 1);
                std::istringstream iss(val);
                double myval = 0;
                iss >> myval;
                if (key == "latitude") {
                    platitude = myval;
                } else if (key == "longitude") {
                    plongitude = myval;
                } else if (key == "altitude") {
                    paltitude = myval;
                } else if (key == "velocity") {
                    pvelocity = myval;
                } else if (key == "heading") {
                    pheading = myval;
                }
            }
        }
    }
}

void PassiveGpsUpdater::stop() {
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mStopRequested = true;
    }
    if (mThread.joinable()) {
        mThread.join();
    }
}

PassiveGpsUpdater::~PassiveGpsUpdater() {
    stop();
}

}  // namespace emulation
}  // namespace android
