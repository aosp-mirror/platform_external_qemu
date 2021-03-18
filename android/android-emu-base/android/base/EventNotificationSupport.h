// Copyright (C) 2021 The Android Open Source Project
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

#include <functional>  // for function
#include <mutex>       // for mutex, lock_guard
#include <utility>     // for move
#include <vector>      // for vector

namespace android {
namespace base {

// An EventNotificationSupport class can be used to add simple notification
// support to your class.
//
// You basically specify an EventObject type, subclass from the support class
// and simply call fireEvent when an event needs to be delivered.
template <class EventObject>
class EventNotificationSupport {
    using EventListener =
            std::function<void(const EventObject evt)>;

public:
    EventNotificationSupport() = default;

    void addListener(EventListener* listener) {
        std::lock_guard<std::mutex> lock(mStreamLock);
        mListeners.push_back(listener);
    }

    void removeListener(EventListener* listener) {
        std::lock_guard<std::mutex> lock(mStreamLock);
        for (auto it = mListeners.begin(); it != mListeners.end();) {
            if (*it == listener) {
                it = mListeners.erase(it);
            } else {
                ++it;
            }
        }
    }

protected:
    void fireEvent(EventObject evt) {
        std::lock_guard<std::mutex> lock(mStreamLock);
        for (const auto& listener : mListeners) {
            (*listener)(evt);
        }
    }

private:
    std::vector<EventListener*> mListeners;
    std::mutex mStreamLock;
};

// A RaiiEventListener is an event listener that will register and unregister
// the provided callback when this object goes out of scope.
template <class Source, class EventObject>
class RaiiEventListener {
public:
    using EventListener =
            std::function<void(const EventObject evt)>;

    RaiiEventListener(Source* src,
                      EventListener listener)
        : mSource(src), mListener(std::move(listener)) {
        mSource->addListener(&mListener);
    }

    ~RaiiEventListener() { mSource->removeListener(&mListener); }

private:
    Source* mSource;
    EventListener mListener;
};
}  // namespace base
}  // namespace android
