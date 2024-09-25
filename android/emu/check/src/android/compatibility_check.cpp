// Copyright (C) 2024 The Android Open Source Project
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
#include "android/emulation/compatibility_check.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "android/avd/info.h"
#include "android/base/logging/StudioMessage.h"

namespace android {
namespace emulation {

AvdCompatibilityManager& AvdCompatibilityManager::instance() {
    static AvdCompatibilityManager sInstance;
    return sInstance;
}

void AvdCompatibilityManager::registerCheck(CompatibilityCheck checkFn,
                                            const char* name) {
    ABSL_LOG_IF(ERROR, name == nullptr) << "You need to provide a name";
    mChecks.emplace_back(std::make_pair<std::string_view, CompatibilityCheck>(
            name, std::move(checkFn)));
}

std::vector<AvdCompatibilityCheckResult> AvdCompatibilityManager::check(
        AvdInfo* avd) {
    LOG(INFO) << "Checking system compatibility:";
    std::vector<AvdCompatibilityCheckResult> results;
    for (auto& [name, check] : mChecks) {
        LOG(INFO) << "  Checking: " << name;
        auto result = check(avd);
        LOG(INFO) << "     " << result.status << ": " << result.description;
        results.emplace_back(result);
    }
    return results;
}

bool AvdCompatibilityManager::hasCompatibilityErrors(
        const std::vector<AvdCompatibilityCheckResult>& results) {
    for (const auto& result : results) {
        if (result.status == AvdCompatibility::Error) {
            return true;  // An error was found, so return true immediately
        }
    }
    return false;  // No errors were found in any of the results
}

void AvdCompatibilityManager::ensureAvdCompatibility(AvdInfo* avd) {
    auto acm = AvdCompatibilityManager::instance();
    auto results = acm.check(avd);

    // Prints the results for android studio.
    acm.printResults(results);
    if (acm.hasCompatibilityErrors(results)) {
        const char* name = avdInfo_getName(avd);
        LOG(FATAL) << "The current system is not compatible with the avd: `"
                   << name << "`, exiting";
        exit(1);
    }
}

void AvdCompatibilityManager::printResults(
        const std::vector<AvdCompatibilityCheckResult>& results) {
    using namespace std::literals::string_view_literals;

    constexpr auto separator = "  "sv;

    std::string error;
    std::string warning;
    for (auto& result : results) {
        switch (result.status) {
            case AvdCompatibility::Ok:
                break;
            case AvdCompatibility::Warning:
                absl::StrAppend(&warning, result.description, separator);
                break;
            case AvdCompatibility::Error:
                absl::StrAppend(&error, result.description, separator);
                break;
        }
    }

    if (!warning.empty())
        USER_MESSAGE(WARNING) << warning;
    if (!error.empty())
        USER_MESSAGE(ERROR) << error;
}

std::vector<std::string_view> AvdCompatibilityManager::registeredChecks() {
    std::vector<std::string_view> result;
    for (const auto& [name, _] : mChecks) {
        result.push_back(name);
    }
    return result;
}

}  // namespace emulation
}  // namespace android