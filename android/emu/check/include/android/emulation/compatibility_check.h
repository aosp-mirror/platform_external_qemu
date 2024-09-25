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
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "absl/strings/str_format.h"
#include "android/avd/info.h"

namespace android {
namespace emulation {
/**
 * @enum AvdCompatibility
 * @brief Represents the compatibility status of an AVD (Android Virtual
 * Device).
 */
enum class AvdCompatibility : uint8_t {
    /**
     * @brief The check succeeded; the AVD is fully compatible.
     */
    Ok = 0,

    /**
     * @brief The AVD can run, but with limited functionality. User should be
     * informed.
     */
    Warning,

    /**
     * @brief The AVD cannot run with the current configuration.
     */
    Error,
};

/**
 * @struct AvdCompatibilityCheckResult
 * @brief Stores the result of an AVD compatibility check, including a
 * description and status.
 */
struct AvdCompatibilityCheckResult {
    /**
     * @brief A description of the check performed and its outcome.
     */
    std::string description;

    /**
     * @brief The AvdCompatibility status indicating the level of compatibility.
     */
    AvdCompatibility status;
};

/**
 * @typedef CompatibilityCheck
 * @brief A function that checks the compatibility of a given AVD with the
 * system.
 * @param avd A pointer to the AvdInfo struct containing information about the
 * AVD.
 * @return An AvdCompatibilityCheckResult struct containing the check outcome.
 */
using CompatibilityCheck = std::function<AvdCompatibilityCheckResult(AvdInfo*)>;

/**
 * @class AvdCompatibilityManager
 * @brief A singleton class managing and executing checks to validate AVD
 * compatibility with the device configuration.
 */
class AvdCompatibilityManager final {
public:
    /**
     * @brief Runs all registered compatibility checks on the specified AVD.
     * @param avd A pointer to the AvdInfo struct representing the AVD to check
     * @return A vector of AvdCompatibilityCheckResult structs, each containing
     * the result of a single check
     */
    std::vector<AvdCompatibilityCheckResult> check(AvdInfo* avd);

    /**
     * @brief Checks the results of AVD compatibility checks for any errors.
     * @param results A vector of AvdCompatibilityCheckResult structs containing
     * the check outcomes.
     * @return true if any of the checks resulted in an AvdCompatibility::Error
     * status, false otherwise.
     */
    bool hasCompatibilityErrors(
            const std::vector<AvdCompatibilityCheckResult>& results);

    /**
     * @brief Prints the results of all compatibility checks in a format
     * suitable for Android Studio.
     * @param results A vector of AvdCompatibilityCheckResult structs containing
     * the check outcomes
     */
    void printResults(const std::vector<AvdCompatibilityCheckResult>& results);

    /**
     * @brief Retrieves the singleton instance of the AvdCompatibilityManager.
     * @return A reference to the single AvdCompatibilityManager instance.
     */
    static AvdCompatibilityManager& instance();

    /**
     * @brief Registers a new compatibility check function with the manager.
     * @param checkFn The compatibility check function to register.
     * @param name A string representing the name of the check.
     */
    void registerCheck(CompatibilityCheck checkFn, const char* name);

    /**
     * @brief Returns a list of the names of all registered compatibility checks
     * @return A vector of string_views, each representing the name of a
     * registered check.
     */
    std::vector<std::string_view> registeredChecks();

   /**
    * @brief Checks the compatibility of an AVD and terminates the program if errors are found
    * @param avd A pointer to the AvdInfo struct representing the AVD to be checked
    */
   static void ensureAvdCompatibility(AvdInfo* avd);
private:
    AvdCompatibilityManager() = default;

    /**
     * @brief Stores the registered checks as pairs of name and check function
     */
    std::vector<std::pair<std::string_view, CompatibilityCheck>> mChecks;

    // Testing..
    friend class AvdCompatibilityManagerTest;
};

/**
 * @macro REGISTER_COMPATIBILITY_CHECK
 * @brief A macro to conveniently register a compatibility check function at
 * compile time
 *
 * Note: Make sure to define your check in this library, or make sure that
 * the library that uses this macro is compiled with the whole archive flag.
 * @param check_name The name of the check function to be registered
 */
#define REGISTER_COMPATIBILITY_CHECK(check_name)                        \
    __attribute__((constructor)) void register##check_name() {          \
        AvdCompatibilityManager::instance().registerCheck(check_name,   \
                                                          #check_name); \
    }

template <typename Sink>
void AbslStringify(Sink& sink, AvdCompatibility status) {
    switch (status) {
        case AvdCompatibility::Ok:
            absl::Format(&sink, "Ok");
            break;
        case AvdCompatibility::Warning:
            absl::Format(&sink, "Warning");
            break;
        case AvdCompatibility::Error:
            absl::Format(&sink, "Error");
            break;
        default:
            ABSL_UNREACHABLE();
    }
}

}  // namespace emulation
}  // namespace android