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

#include "android/automation/AutomationController.h"
#include "android/automation/AutomationEventSink.h"
#include "android/base/memory/LazyInstance.h"

#include <google/protobuf/text_format.h>

#include <memory>

using namespace android::base;

namespace android {
namespace automation {

class AutomationControllerImpl : public AutomationController {
public:
    AutomationControllerImpl();

    // Get the event sink for recording automation events.
    AutomationEventSink& getEventSink() override;

private:
    std::unique_ptr<AutomationEventSink> mEventSink;
};

AutomationController& AutomationController::get() {
    static LazyInstance<AutomationControllerImpl> sInstance;
    return sInstance.get();
}

AutomationControllerImpl::AutomationControllerImpl()
    : mEventSink(new AutomationEventSink()) {}

AutomationEventSink& AutomationControllerImpl::getEventSink() {
    return *mEventSink;
}

}  // namespace automation
}  // namespace android
