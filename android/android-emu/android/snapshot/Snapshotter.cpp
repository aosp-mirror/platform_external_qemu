// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/Snapshotter.h"

#include "android/base/memory/LazyInstance.h"
#include "android/crashreport/CrashReporter.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/StudioConfig.h"
#include "android/snapshot/interface.h"
#include "android/snapshot/Quickboot.h"
#include "android/snapshot/Hierarchy.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

#include <cassert>
#include <utility>

extern "C" {
#include <emmintrin.h>
}

using android::base::LazyInstance;
using android::base::System;
using android::crashreport::CrashReporter;

// Inspired by QEMU's bufferzero.c implementation, but simplified for the case
// when checking the whole aligned memory page.
static bool buffer_zero_sse2(const void* buf, int len) {
    buf = __builtin_assume_aligned(buf, 1024);
    __m128i t = _mm_load_si128(static_cast<const __m128i*>(buf));
    auto p = reinterpret_cast<__m128i*>(
            (reinterpret_cast<intptr_t>(buf) + 5 * 16));
    auto e =
            reinterpret_cast<__m128i*>((reinterpret_cast<intptr_t>(buf) + len));
    const __m128i zero = _mm_setzero_si128();

    /* Loop over 16-byte aligned blocks of 64.  */
    do {
        __builtin_prefetch(p);
        t = _mm_cmpeq_epi32(t, zero);
        if (_mm_movemask_epi8(t) != 0xFFFF) {
            return false;
        }
        t = p[-4] | p[-3] | p[-2] | p[-1];
        p += 4;
    } while (p <= e);

    /* Finish the aligned tail.  */
    t |= e[-3];
    t |= e[-2];
    t |= e[-1];
    return _mm_movemask_epi8(_mm_cmpeq_epi32(t, zero)) == 0xFFFF;
}

namespace android {
namespace snapshot {

static const System::Duration kSnapshotCrashThresholdMs = 120000; // 2 minutes

// TODO: implement an optimized SSE4 version and dynamically select it if host
// supports SSE4.
bool isBufferZeroed(const void* ptr, int32_t size) {
    assert((uintptr_t(ptr) & (1024 - 1)) == 0);  // page-aligned
    assert(size >= 1024);  // at least one small page in |size|
    return buffer_zero_sse2(ptr, size);
}

Snapshotter::Snapshotter() : mCallback([](Operation, Stage) {}) {}

Snapshotter::~Snapshotter() {
    if (mVmOperations.setSnapshotCallbacks) {
        mVmOperations.setSnapshotCallbacks(nullptr, nullptr);
    }
}

static LazyInstance<Snapshotter> sInstance = {};

Snapshotter& Snapshotter::get() {
    return sInstance.get();
}

void Snapshotter::initialize(const QAndroidVmOperations& vmOperations,
                             const QAndroidEmulatorWindowAgent& windowAgent) {
    static const SnapshotCallbacks kCallbacks = {
            // ops
            {
                    // save
                    {// start
                     [](void* opaque, const char* name) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         return snapshot->onStartSaving(name) ? 0 : -1;
                     },
                     // end
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onSavingComplete(name, res);
                     },
                     // quick fail
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onSavingFailed(name, res);
                     }},
                    // load
                    {// start
                     [](void* opaque, const char* name) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         return snapshot->onStartLoading(name) ? 0 : -1;
                     },
                     // end
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onLoadingComplete(name, res);
                     },  // quick fail
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onLoadingFailed(name, res);
                     }},
                    // del
                    {// start
                     [](void* opaque, const char* name) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         return snapshot->onStartDelete(name) ? 0 : -1;
                     },
                     // end
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onDeletingComplete(name, res);
                     },
                     // quick fail
                     [](void*, const char*, int) {}},
            },
            // ramOps
            {// registerBlock
             [](void* opaque, SnapshotOperation operation,
                const RamBlock* block) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);
                 if (operation == SNAPSHOT_LOAD) {
                     snapshot->mLoader->ramLoader().registerBlock(*block);
                 } else if (operation == SNAPSHOT_SAVE) {
                     snapshot->mSaver->ramSaver().registerBlock(*block);
                 }
             },
             // startLoading
             [](void* opaque) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);
                 snapshot->mLoader->ramLoader().start(snapshot->isQuickboot());
                 return snapshot->mLoader->ramLoader().hasError() ? -1 : 0;
             },
             // savePage
             [](void* opaque, int64_t blockOffset, int64_t pageOffset,
                int32_t size) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);
                 snapshot->mSaver->ramSaver().savePage(blockOffset, pageOffset,
                                                       size);
             },
             // savingComplete
             [](void* opaque) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);
                 snapshot->mSaver->ramSaver().join();
                 return snapshot->mSaver->ramSaver().hasError() ? -1 : 0;
             },
             // loadRam
             [](void* opaque, void* hostRamPtr, uint64_t size) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);

                 auto& loader = snapshot->mLoader;
                 if (!loader || loader->status() != OperationStatus::Ok) return;

                 auto& ramLoader = loader->ramLoader();
                 if (ramLoader.onDemandEnabled() &&
                     !ramLoader.onDemandLoadingComplete()) {
                     ramLoader.loadRam(hostRamPtr, size);
                 }
             }}};

    assert(vmOperations.setSnapshotCallbacks);
    mVmOperations = vmOperations;
    mWindowAgent = windowAgent;
    mVmOperations.setSnapshotCallbacks(this, &kCallbacks);
}  // namespace snapshot

OperationStatus Snapshotter::prepareForLoading(const char* name) {
    if (mSaver && mSaver->snapshot().name() == name) {
        mSaver.clear();
    }
    mLoader.emplace(name);
    mLoader->prepare();
    return mLoader->status();
}

OperationStatus Snapshotter::load(bool isQuickboot, const char* name) {
    mIsQuickboot = isQuickboot;
    mVmOperations.snapshotLoad(name, this, nullptr);
    mIsQuickboot = false;
    mLoadedSnapshotFile = (mLoader->status() == OperationStatus::Ok) ? name : "";
    return mLoader->status();
}

OperationStatus Snapshotter::prepareForSaving(const char* name) {
    if (mLoader && mLoader->snapshot().name() == name) {
        mLoader.clear();
    }
    mSaver.emplace(name);
    mSaver->prepare();
    return mSaver->status();
}

OperationStatus Snapshotter::save(const char* name) {
    mVmOperations.snapshotSave(name, this, nullptr);
    return mSaver->status();
}

void Snapshotter::deleteSnapshot(const char* name) {
    if (!strcmp(name, mLoadedSnapshotFile.c_str())) {
        // We're deleting the "loaded" snapshot
        mLoadedSnapshotFile.clear();
    }
    mVmOperations.snapshotDelete(name, this, nullptr);
}

void Snapshotter::onCrashedSnapshot(const char* name) {
    // if it's been less than 2 minutes since the load,
    // consider it a snapshot fail.
    if (System::Duration(System::get()->getProcessTimes().wallClockMs) -
        mLastLoadUptimeMs < kSnapshotCrashThresholdMs) {
        onLoadingFailed(name, -EINVAL);
    }
}

bool Snapshotter::onStartSaving(const char* name) {
    CrashReporter::get()->hangDetector().pause(true);
    mCallback(Operation::Save, Stage::Start);
    mLoader.clear();
    if (!mSaver || isComplete(*mSaver)) {
        mSaver.emplace(name);
    }
    if (mSaver->status() == OperationStatus::Error) {
        onSavingComplete(name, -1);
        return false;
    }
    return true;
}

bool Snapshotter::onSavingComplete(const char* name, int res) {
    assert(mSaver && name == mSaver->snapshot().name());
    mSaver->complete(res == 0);
    CrashReporter::get()->hangDetector().pause(false);
    mCallback(Operation::Save, Stage::End);
    bool good = mSaver->status() != OperationStatus::Error;
    if (good) {
        Hierarchy::get()->currentInfo();
    }
    return good;
}

void Snapshotter::onSavingFailed(const char* name, int res) {
    // Well, we haven't started anything and it failed already - nothing to do.
}

bool Snapshotter::onStartLoading(const char* name) {
    mLoadedSnapshotFile.clear();
    CrashReporter::get()->hangDetector().pause(true);
    mCallback(Operation::Load, Stage::Start);
    mSaver.clear();
    if (!mLoader || isComplete(*mLoader)) {
        if (mLoader) {
            mLoader->interrupt();
        }
        mLoader.emplace(name);
    }
    mLoader->start();
    if (mLoader->status() == OperationStatus::Error) {
        onLoadingComplete(name, -1);
        return false;
    }
    return true;
}

bool Snapshotter::onLoadingComplete(const char* name, int res) {
    assert(mLoader && name == mLoader->snapshot().name());
    mLoader->complete(res == 0);
    CrashReporter::get()->hangDetector().pause(false);
    mLastLoadUptimeMs =
            System::Duration(System::get()->getProcessTimes().wallClockMs);
    mCallback(Operation::Load, Stage::End);
    if (mLoader->status() == OperationStatus::Error) {
        return false;
    }
    mLoadedSnapshotFile = name;
    Hierarchy::get()->currentInfo();
    return true;
}

void Snapshotter::onLoadingFailed(const char* name, int err) {
    assert(err < 0);
    mSaver.clear();
    if (err == -EINVAL) { // corrupted snapshot. abort immediately,
                          // try not to do anything since this could be
                          // in the crash handler
        mLoader->onInvalidSnapshotLoad();
        return;
    }
    mLoader.emplace(name, -err);
    mLoader->complete(false);
    mLoadedSnapshotFile.clear();
}

bool Snapshotter::onStartDelete(const char*) {
    CrashReporter::get()->hangDetector().pause(true);
    return true;
}

bool Snapshotter::onDeletingComplete(const char* name, int res) {
    if (res == 0) {
        if (mSaver && mSaver->snapshot().name() == name) {
            mSaver.clear();
        }
        if (mLoader && mLoader->snapshot().name() == name) {
            mLoader.clear();
        }
        path_delete_dir(Snapshot::dataDir(name).c_str());
    }
    CrashReporter::get()->hangDetector().pause(false);
    return true;
}

void Snapshotter::setOperationCallback(Callback&& cb) {
    mCallback = bool(cb) ? std::move(cb) : [](Operation, Stage) {};
}

}  // namespace snapshot
}  // namespace android

void androidSnapshot_initialize(
        const QAndroidVmOperations* vmOperations,
        const QAndroidEmulatorWindowAgent* windowAgent) {
    using android::base::Version;

    static constexpr auto kMinStudioVersion = Version(3, 0, 0);
    // Make sure the installed AndroidStudio is able to handle the snapshots
    // feature.
    if (isEnabled(android::featurecontrol::FastSnapshotV1)) {
        auto version = android::studio::lastestAndroidStudioVersion();
        if (version.isValid() && version < kMinStudioVersion) {
            auto prettyVersion = Version(version.component<Version::kMajor>(),
                                         version.component<Version::kMinor>(),
                                         version.component<Version::kMicro>());
            VERBOSE_PRINT(init,
                          "Disabling snapshot boot - need Android Studio %s "
                          " but found %s",
                          kMinStudioVersion.toString().c_str(),
                          prettyVersion.toString().c_str());
            setEnabledOverride(android::featurecontrol::FastSnapshotV1, false);
        }
    }

    android::snapshot::sInstance->initialize(*vmOperations, *windowAgent);
    android::snapshot::Quickboot::initialize(*vmOperations, *windowAgent);
}

void androidSnapshot_finalize() {
    android::snapshot::Quickboot::finalize();
    android::snapshot::sInstance->~Snapshotter();
}
