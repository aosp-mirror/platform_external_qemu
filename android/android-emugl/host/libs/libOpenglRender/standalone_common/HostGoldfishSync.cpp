#include "HostGoldfishSync.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/GoldfishSyncCommandQueue.h"
#include "android/emulation/goldfish_sync.h"
#include "android/emulation/testing/TestVmLock.h"

#include <atomic>
#include <functional>
#include <unordered_set>

#include <string.h>

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::LazyInstance;
using android::base::Lock;

namespace emugl {

static trigger_wait_fn_t sTriggerWaitFn = nullptr;

class SyncDevice {
public:
    SyncDevice() = default;

    int currTimelinePt() { return mTimelinePt.load(); }

    void incTimeline(int howmuch) {
        mTimelinePt.fetch_add(howmuch);
        AutoLock lock(mLock);
        mFences.erase(
            std::remove_if(
                mFences.begin(), mFences.end(),
                [this](const Fence& f) {
                    return f.pt <= mTimelinePt.load();
                }),
            mFences.end());
        mCv.broadcast();
    }

    int createFence(int pt) {
        AutoLock lock(mLock);
        int newId = nextUnusedFenceIdLocked();
        mFences.push_back({ newId, pt });
    }

    void waitFence(int fence) {
        AutoLock lock(mLock);
        while (fenceExistsLocked(fence)) {
            mCv.wait(&mLock);
        }
    }

private:
    std::atomic<int> mTimelinePt = { 0 };

    struct Fence {
        int id;
        int pt;
    };

    Lock mLock;
    ConditionVariable mCv;
    std::vector<Fence> mFences = {};

    int nextUnusedFenceIdLocked() const {
        int res = 0;
        for (const auto& fence : mFences) {
            if (res <= fence.id) res = fence.id + 1;
        }
        return res;
    }

    bool fenceExistsLocked(int fence) const {
        for (const auto& f : mFences) {
            if (f.id == fence) return true;
        }
        return false;
    }

};

static LazyInstance<SyncDevice> sDevice = LAZY_INSTANCE_INIT;


static GoldfishSyncDeviceInterface kSyncDeviceInterface = {
    .doHostCommand = [](unsigned int, unsigned long long, unsigned int, unsigned long long) { fprintf(stderr, "%s: nyi\n", __func__); },
    .registerTriggerWait = [](trigger_wait_fn_t fn) {
        sTriggerWaitFn = fn;
    },
};

void host_goldfish_sync_init() {
    fprintf(stderr, "%s: call\n", __func__);
    sDevice.get();
    // goldfish_sync_set_hw_funcs(&kSyncDeviceInterface);
}

int host_goldfish_sync_create_fence() {
    return sDevice->createFence(sDevice->currTimelinePt() + 1);
}

void host_goldfish_sync_trigger_wait(uint64_t glsync_handle, uint64_t syncthread_handle) {
    sTriggerWaitFn(glsync_handle, syncthread_handle, 0);
}

void host_goldfish_sync_wait_fence(int fence) {
    sDevice->waitFence(fence);
}

} // namespace emugl
