// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/base/Compiler.h"
#include "android/base/memory/LazyInstance.h"

#include <memory>

namespace android {
namespace automation {

// Controls recording and playback of emulator automation events.

// Forward declarations.
class AutomationEventSink;

class AutomationController {
    DISALLOW_COPY_AND_ASSIGN(AutomationController);
    friend class android::base::LazyInstance<AutomationController>;

    // AutomationController is a singleton, use get() to get an instance.
    AutomationController();

public:
    // Get the global instance of the AutomationController.
    static AutomationController& get();

    // Get the event sink for recording automation events.
    AutomationEventSink& getEventSink();

private:
    std::unique_ptr<AutomationEventSink> mEventSink;
};

}  // namespace automation
}  // namespace android
