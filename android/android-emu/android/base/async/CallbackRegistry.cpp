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
#include "android/base/async/CallbackRegistry.h"

#include <new>      // for operator new
#include <utility>  // for pair, move
#include <vector>   // for vector

/* set >0 for very verbose debugging */
#define DEBUG 0
#if DEBUG >= 1
#define DD(fmt, ...)                                                          \
    fprintf(stderr, "CallbackRegistry: %s:%d| " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace base {

CallbackRegistry::~CallbackRegistry() {
    close();
}

void CallbackRegistry::close() {
    mMessages.stop();
}

void CallbackRegistry::registerCallback(
        MessageAvailableCallback messageAvailable,
        void* opaque) {
    ForwarderMessage msg = {
            .cmd = MESSAGE_FORWARDER_ADD,
            .data = opaque,
            .callback = messageAvailable,
    };

    mMessages.send(msg);
}

void CallbackRegistry::unregisterCallback(void* opaque) {
    ForwarderMessage msg = {
            .cmd = MESSAGE_FORWARDER_REMOVE,
            .data = opaque,
            .callback = {},
    };
    mMessages.send(msg);
    AutoLock removeLock(mLock);
    mCvRemoval.wait(&mLock, [=]() {
        return mProcessing == 0 || mStoredForwarders.count(msg.data) == 0;
    });
}

void CallbackRegistry::invokeCallbacks() {
    mProcessing++;
    ForwarderMessage msg;
    DD("Process: %d", mProcessing.load());
    while (mMessages.tryReceive(&msg)) {
        switch (msg.cmd) {
            case MESSAGE_FORWARDER_ADD: {
                AutoLock lock(mLock);
                mStoredForwarders[msg.data] = std::move(msg.callback);
                break;
            }
            case MESSAGE_FORWARDER_REMOVE: {
                mLock.lock();
                mStoredForwarders.erase(msg.data);
                mCvRemoval.signalAndUnlock(&mLock);
                break;
            }
            default:
                break;
        }
    }

    struct RegisteredForwarder {
        void* data;
        MessageAvailableCallback callback;
    };

    std::vector<RegisteredForwarder> toRun;

    {
        AutoLock lock(mLock);
        for (auto it : mStoredForwarders) {
            RegisteredForwarder rf = {
                    .data = it.first,
                    .callback = it.second,
            };
            toRun.push_back(rf);
        }
    }

    for (const auto& rf : toRun) {
        rf.callback(rf.data);
    }

    mProcessing--;
    mLock.lock();
    mCvRemoval.signalAndUnlock(&mLock);
}

}  // namespace base
}  // namespace android
