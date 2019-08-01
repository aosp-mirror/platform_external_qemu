// Copyright 2019 The Android Open Source Project
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
#include "android/base/RateEstimator.h"

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"

#include <string>
#include <stdio.h>

namespace android {
namespace base {

class RateEstimator::Impl {
public:
    Impl(const char* tag, RateEstimator::IntervalUs samplingInterval) :
        mTag(tag),
        mSamplingInterval(samplingInterval ? samplingInterval : 1),
        mWorker([this] { worker(); }) { }

    ~Impl() {
        mExiting = true;
        mWorker.wait();
    }

    void addEvent() {
        ++mEventCounter;
    }

    float getHz() const {
        return (float)(mEventsSinceLastInterval) / (mSamplingInterval * 1000000.0f);
    }

    void worker() {
        AutoLock lock(mLock);

        while (!mExiting) {
            auto currEvents = mEventCounter;
            const auto now = System::get()->getUnixTimeUs();
            const auto deadline = now + mSamplingInterval;
            mCv.timedWait(&mLock, deadline);
            auto nextEvents = mEventCounter;
            mEventsSinceLastInterval = nextEvents - currEvents;
        }
    }

    void report() {
        printf("RateEstimator[%s]: %f Hz\n", mTag.c_str(), getHz());
    }

private:
    std::string mTag;
    RateEstimator::IntervalUs mSamplingInterval = 1;
    FunctorThread mWorker;

    // Not under any lock
    volatile uint32_t mEventCounter = 0;
    volatile uint32_t mEventsSinceLastInterval = 0;

    // Locked state
    Lock mLock;
    ConditionVariable mCv;
    bool mExiting = false;
};

RateEstimator::RateEstimator(const char* tag, IntervalUs samplingInterval) :
    mImpl(new RateEstimator::Impl(tag, samplingInterval)) { }

RateEstimator::~RateEstimator() = default;

void RateEstimator::addEvent() {
    mImpl->addEvent();
}

float RateEstimator::getHz() const {
    return mImpl->getHz();
}

void RateEstimator::report() {
    mImpl->report();
}

} // namespace android
} // namespace base
