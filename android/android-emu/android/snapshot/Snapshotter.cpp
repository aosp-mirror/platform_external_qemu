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

#include "android/crashreport/CrashReporter.h"
#include "android/snapshot/interface.h"
#include "android/utils/path.h"

#include <assert.h>

extern "C" {
#include <emmintrin.h>
}

// Inspired by QEMU's bufferzero.c implementation, but simplified for the case
// when checking the whole aligned memory page.
static bool buffer_zero_sse2(const void* buf, int len) {
    buf = __builtin_assume_aligned(buf, 4096);
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

const SnapshotCallbacks Snapshotter::kCallbacks = {
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
                 }},
        },
        // ramOps
        {// registerBlock
         [](void* opaque, SnapshotOperation operation, const RamBlock* block) {
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
             snapshot->mLoader->ramLoader().start();
             return snapshot->mLoader->ramLoader().hasError() ? -1 : 0;
         },
         // savePage
         [](void* opaque,
            int64_t blockOffset,
            int64_t pageOffset,
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
         }}};

// TODO: implement an optimized SSE4 version and dynamically select it if host
// supports SSE4.
bool isBufferZeroed(const void* ptr, int32_t size) {
    assert((uintptr_t(ptr) & (4096 - 1)) == 0); // page-aligned
    assert(size >= 4096);  // at least one small page in |size|
    return buffer_zero_sse2(ptr, size);
}

Snapshotter::Snapshotter(const QAndroidVmOperations& vmOperations)
    : mVmOperations(vmOperations) {
    mVmOperations.setSnapshotCallbacks(this, &kCallbacks);
}

Snapshotter::~Snapshotter() {
    mVmOperations.setSnapshotCallbacks(nullptr, nullptr);
}

static Snapshotter* sSnapshotter = nullptr;
Snapshotter& Snapshotter::get() {
    assert(sSnapshotter != nullptr);
    return *sSnapshotter;
}

OperationStatus Snapshotter::prepareForLoading(const char* name) {
    if (mSaver && mSaver->snapshot().name() == name) {
        mSaver.clear();
    }
    mLoader.emplace(name);
    mLoader->prepare();
    return mLoader->status();
}

OperationStatus Snapshotter::load(const char* name) {
    mVmOperations.snapshotLoad(name, this, nullptr);
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

bool Snapshotter::onStartSaving(const char* name) {
    crashreport::CrashReporter::get()->hangDetector().pause(true);
    mLoader.clear();
    if (!mSaver || isComplete(*mSaver)) {
        mSaver.emplace(name);
    }
    return mSaver->status() != OperationStatus::Error;
}

bool Snapshotter::onSavingComplete(const char* name, int res) {
    assert(mSaver && name == mSaver->snapshot().name());
    mSaver->complete(res == 0);
    crashreport::CrashReporter::get()->hangDetector().pause(false);
    return mSaver->status() != OperationStatus::Error;
}

bool Snapshotter::onStartLoading(const char* name) {
    crashreport::CrashReporter::get()->hangDetector().pause(true);
    mSaver.clear();
    if (!mLoader || isComplete(*mLoader)) {
        mLoader.emplace(name);
    }
    mLoader->start();
    return mLoader->status() != OperationStatus::Error;
}

bool Snapshotter::onLoadingComplete(const char* name, int res) {
    assert(mLoader && name == mLoader->snapshot().name());
    mLoader->complete(res == 0);
    crashreport::CrashReporter::get()->hangDetector().pause(false);
    return mLoader->status() != OperationStatus::Error;
}

bool Snapshotter::onStartDelete(const char*) {
    crashreport::CrashReporter::get()->hangDetector().pause(true);
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
    crashreport::CrashReporter::get()->hangDetector().pause(false);
    return true;
}

}  // namespace snapshot
}  // namespace android

void androidSnapshot_initialize(const QAndroidVmOperations* vmOperations) {
    assert(android::snapshot::sSnapshotter == nullptr);
    android::snapshot::sSnapshotter =
            new android::snapshot::Snapshotter(*vmOperations);
}

void androidSnapshot_finalize() {
    delete android::snapshot::sSnapshotter;
    android::snapshot::sSnapshotter = nullptr;
}
