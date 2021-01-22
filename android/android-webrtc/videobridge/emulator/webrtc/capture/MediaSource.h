// Copyright (C) 2019 The Android Open Source Project
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
#include <rtc_base/logging.h>  // for Val, RTC_LOG
#include <rtc_base/ref_count.h>
#include <memory>
#include <mutex>
#include <thread>

namespace emulator {
namespace webrtc {

// A MediaSource is a source for audio or video, that can be started
// and stopped. A media source can be used by multiple tracks, so usually you
// only want one media source object active at all times. The MediaSource can be
// obtained from the MediaSourceLibrary which manages the lifetime of the
// underlying source.
//
// It provides an implements the RefCountInterface in terms of starting
// and stopping the source.
//
// This makes it easy to use in the WebRTC api, as the source will be stopped
// when it goes out of scope when it is no longer in use.
template <class T>
class MediaSource : public T {
public:
    MediaSource() {}

    template <class P0>
    explicit MediaSource(P0&& p0) : T(std::forward<P0>(p0)) {}

    template <class P0, class P1, class... Args>
    MediaSource(P0&& p0, P1&& p1, Args&&... args)
        : T(std::forward<P0>(p0),
            std::forward<P1>(p1),
            std::forward<Args>(args)...) {}

    void AddRef() const override {
        const_cast<MediaSource<T>*>(this)->start();
    };

    virtual rtc::RefCountReleaseStatus Release() const override {
        const_cast<MediaSource<T>*>(this)->stop();
        return rtc::RefCountReleaseStatus::kOtherRefsRemained;
    };

private:
    void start() {
        auto active = mActive.fetch_add(1, std::memory_order_relaxed) + 1;
        if (active == 1) {
            mProducerThread =
                    std::make_unique<std::thread>([this]() { T::run(); });
        }
    }

    void stop() {
        int active = mActive.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (active == 0) {
            T::cancel();
            mProducerThread->join();
        }
    }

    std::unique_ptr<std::thread> mProducerThread;
    mutable std::atomic<int> mActive{0};
};
}  // namespace webrtc
}  // namespace emulator
