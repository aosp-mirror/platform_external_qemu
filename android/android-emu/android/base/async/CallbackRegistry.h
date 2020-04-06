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

#include <atomic>         // for atomic_int
#include <unordered_map>  // for unordere...

#include "android/base/synchronization/ConditionVariable.h"  // for Conditio...
#include "android/base/synchronization/Lock.h"               // for Lock
#include "android/base/synchronization/MessageChannel.h"     // for MessageC...

namespace android {
namespace base {

typedef void (*MessageAvailableCallback)(void* opaque);

// A CallbackRegistry can be used to (un)register callbacks, the callbacks
// are invoked when the invokeCallbacks is called. A callback is guaranteed
// never to be called once unregisterCallback is returned.
//
// The expectation is that the invokeCallbacks method is called relatively
// frequently, since registration only really happens when this method is
// invoked. Unregister can block if processing is currently take place to
// guarantee that a callback is never called once unregisterCallback returns.
//
// Only one thread can be actively processing invokeCallbacks, other threads
// will block and wait until invokeCallbacks has completed.
//
// The MessageAvailableCallback functions should not block too long, as this can
// cause blocking on the unregister method and invokeCallbacks.
//
// It is expected that registration and procesessing happen on
// two separate threads.
class CallbackRegistry {
public:
    CallbackRegistry() = default;
    ~CallbackRegistry();

    // Registers the given callback under the `opaque` key. This does not block,
    // unless the registration queue is filled up. |messageAvailable| Callback
    // to register. |opaque| Key under which this callback is registered.
    void registerCallback(MessageAvailableCallback messageAvailable,
                          void* opaque);

    // Unregister the callback identiefied by the `opaque` key. You are
    // guaranteed that the associated callback will not be invoked after this
    // function returns. This call  can block if:
    // - processing is currently taking place.
    // - the message queue is filled to capacity.
    //
    // You can safely delete opaque upon return.
    //
    // |opaque| Key under which this callback is registered.
    void unregisterCallback(void* opaque);

    // This will process all the registrations and invoke the callbacks.
    void invokeCallbacks();

    // Close the registry, cancelling any pending registrations.
    void close();

    // Number of register/unregister slots available before blocking.
    // (Mainly used by unit tests to make sure we do not overrun the queue)
    size_t available();

    // Maximum number of slots
    size_t max_slots() { return MAX_QUEUE_SIZE; };

    // true if we are currently processing requests.
    bool processing() { return mProcessing; }

private:
    enum MessageForwarderCommand {
        MESSAGE_FORWARDER_ADD = 0,
        MESSAGE_FORWARDER_REMOVE = 1,
        MESSAGE_UNKNOWN = 2,
    };

    struct ForwarderMessage {
        MessageForwarderCommand cmd;
        void* data;
        MessageAvailableCallback callback;
    };

    void updateForwarders();

    static const size_t MAX_QUEUE_SIZE = 64;
    android::base::MessageChannel<ForwarderMessage, MAX_QUEUE_SIZE> mMessages;
    android::base::Lock mLock;            // protects mStoredForwarders
    android::base::Lock mProcessingLock;  // protects processing.
    android::base::ConditionVariable mCvRemoval;
    std::unordered_map<void*, MessageAvailableCallback> mStoredForwarders;
    bool mProcessing{false};
};

}  // namespace base
}  // namespace android
