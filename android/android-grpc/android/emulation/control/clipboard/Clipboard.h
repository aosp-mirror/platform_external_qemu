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
#pragma once

#include <stddef.h>                                       // for size_t
#include <stdint.h>                                       // for uint8_t
#include <string>                                         // for string, bas...

#include "android/emulation/control/clipboard_agent.h"    // for QAndroidCli...
#include "android/emulation/control/utils/EventWaiter.h"  // for EventWaiter
#include "android/base/synchronization/Lock.h"

namespace android {
namespace emulation {
namespace control {

// Contains a set of convenience methods to assist with interacting
// with the emulator clipboard.
//
// The Clipboard will register a callback with the android agent and
// will update itself accordingly.
class Clipboard {
public:
    // Send the clipboard data to the emulator.
    void sendContents(const std::string& clipdata);

    // Retrieves the current clipboard state
    std::string getCurrentContents();

    // Gets the event waiter that can be used to wait for new
    // clipboard updates.
    EventWaiter* eventWaiter();

    // Gets the clipboard singleton that will register with the given agent.
    // Note: For unit tests only the first registered agent will be used.
    static Clipboard* getClipboard(const QAndroidClipboardAgent* clipAgent);
private:

    Clipboard(const QAndroidClipboardAgent* clipAgent);
    static void guestClipboardCallback(void* context, const uint8_t* data, size_t size);
    std::string mContents;
    const QAndroidClipboardAgent* mClipAgent;
    EventWaiter mEventWaiter;
    android::base::Lock mContentsLock;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
