// Copyright 2022 The Android Open Source Project
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
#include <set>

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/process_state.h"

namespace google_breakpad {
class ProcessState;
}  // namespace google_breakpad

namespace android {
namespace crashreport {
// The Suggestion type represents possible suggestions
// presented to the user upon
// analyzing a crash report.
enum class Suggestion {
    // TODO: Add more suggestion types
    // (Update OS, Get more RAM, etc)
    // as we find them
    UpdateGfxDrivers
};

class UserSuggestions {
public:
    std::set<Suggestion> suggestions;
    std::set<const char*> stringSuggestions;

    UserSuggestions(google_breakpad::ProcessState* process_state);
};
}  // namespace crashreport
}  // namespace android