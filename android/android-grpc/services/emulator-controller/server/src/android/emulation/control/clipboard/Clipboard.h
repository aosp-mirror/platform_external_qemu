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

#include <grpcpp/grpcpp.h>
#include <stddef.h>
#include <stdint.h>
#include <mutex>
#include <string>

#include "android/emulation/control/clipboard_agent.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

struct ClipboardEvent {
    std::string source;
    ClipData data;
};

// Contains a set of convenience methods to assist with interacting
// with the emulator clipboard.
//
// The Clipboard will register a callback with the android agent and
// will update itself accordingly.
class Clipboard : public EventChangeSupport<ClipboardEvent> {
public:
    // Send the clipboard data to the emulator.
    void sendContents(const std::string& clipdata,
                      const ::grpc::ServerContextBase* from);

    // Retrieves the current clipboard state
    std::string getCurrentContents();

    // Gets the clipboard singleton that will register with the given agent.
    // Note: For unit tests only the first registered agent will be used.
    static Clipboard* getClipboard(const QAndroidClipboardAgent* clipAgent);

    grpc::ServerWriteReactor<ClipData>* eventWriter(
            ::grpc::CallbackServerContext* context);

private:
    Clipboard(const QAndroidClipboardAgent* clipAgent);
    static void guestClipboardCallback(void* context,
                                       const uint8_t* data,
                                       size_t size);
    std::string mContents;
    const QAndroidClipboardAgent* mClipAgent;
    std::mutex mContentsLock;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
