// Copyright 2018 The Android Open Source Project
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
#include "android/base/CpuUsage.h"

#include "android/base/async/Looper.h"
#include "android/base/async/RecurrentTask.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"

#include <array>

namespace android {
namespace base {

class CpuUsage::Impl {
public:
    Impl() {}

    struct LooperMeasurement {
        Looper* looper = nullptr;
        RecurrentTask* task = nullptr;
        CpuTime cpuTime;
        CpuTime lastMeasurement;
    };

    void stop() {
        AutoWriteLock lock(mLock);
        for (auto& m : mMeasurements) {
            if (m.task) {
                m.task->stopAndWait();
            }
        }
    }

    ~Impl() {
        stop();
        AutoWriteLock lock(mLock);
        for (auto& m : mMeasurements) {
            if (m.task) {
                delete m.task;
            }
        }

    }

    void addLooper(int usageArea, Looper* looper) {
        if (usageArea >= mMeasurements.size()) return;

        AutoWriteLock lock(mLock);
        mMeasurements[usageArea].looper = looper;
        mMeasurements[usageArea].task =
            new RecurrentTask(
                looper, [this, usageArea] { return doMeasurement(usageArea); },
                /* TODO: add microsecond precision to RecurrentTask */
                mMeasurementIntervalUs / 1000);
        mMeasurements[usageArea].task->start();
    }

    void setEnabled(bool enable) {
        AutoWriteLock lock(mLock);
        mEnabled = enable;
    }

    void setMeasurementInterval(CpuUsage::IntervalUs interval) {
        AutoWriteLock lock(mLock);
        mMeasurementIntervalUs = interval;
        for (auto& m : mMeasurements) {
            if (m.task) {
                m.task->setIntervalMs(
                    mMeasurementIntervalUs / 1000);
            }
        }
    }

    void forEachMeasurement(int start, int end, CpuUsage::CpuTimeReader func) {
        AutoReadLock lock(mLock);
        for (int i = start; i < end; ++i) {
            if (!mMeasurements[i].looper) return;
            func(mMeasurements[i].lastMeasurement);
        }
    }

private:

    bool doMeasurement(int usageArea) {
        AutoReadLock lock(mLock);
        if (!mEnabled) return true;
        lock.unlockRead();

        auto& prevTime = mMeasurements[usageArea].cpuTime;
        auto& lastMeasurement = mMeasurements[usageArea].lastMeasurement;

        CpuTime nextMeasurement = System::cpuTime();
        lastMeasurement = nextMeasurement - prevTime;
        prevTime = nextMeasurement;

        return true;
    }

    std::array<LooperMeasurement, CpuUsage::UsageArea::Max> mMeasurements = {};
    bool mEnabled = false;
    CpuUsage::IntervalUs mMeasurementIntervalUs = 1000;
    ConditionVariable mWorkerThreadCv;
    ReadWriteLock mLock;
};

CpuUsage::CpuUsage() : mImpl(new CpuUsage::Impl()) { }

void CpuUsage::addLooper(int usageArea, Looper* looper) {
    mImpl->addLooper(usageArea, looper);
}

void CpuUsage::setEnabled(bool enable) {
    mImpl->setEnabled(enable);
}

void CpuUsage::setMeasurementInterval(CpuUsage::IntervalUs interval) {
    mImpl->setMeasurementInterval(interval);
}

void CpuUsage::forEachUsage(UsageArea area, CpuTimeReader readerFunc) {
    int start;

    if (area >= UsageArea::MainLoop && area < UsageArea::Vcpu) {
        mImpl->forEachMeasurement(UsageArea::MainLoop, UsageArea::Vcpu, readerFunc);
    } else if (area >= UsageArea::Vcpu && area < UsageArea::RenderThreads) {
        mImpl->forEachMeasurement(UsageArea::Vcpu, UsageArea::RenderThreads, readerFunc);
    } else if (area >= UsageArea::RenderThreads && area < UsageArea::Max) {
        mImpl->forEachMeasurement(UsageArea::RenderThreads, UsageArea::Max, readerFunc);
    } else {
        fprintf(stderr, "%s: warning: invalid usage area %d detected.\n", __func__, area);
    }
}

} // namespace android
} // namespace base