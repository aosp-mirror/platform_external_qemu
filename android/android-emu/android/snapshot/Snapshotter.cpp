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

#include "android/snapshot/interface.h"
#include "android/crashreport/CrashReporter.h"
#include "android/utils/path.h"

#include <assert.h>

namespace android {
namespace snapshot {

const SnapshotCallbacks Snapshotter::kCallbacks = {
        // ops
        {
                // save
                {// start
                 [](void* opaque, const char* name) {
                     auto snapshot = static_cast<Snapshotter*>(opaque);
                     snapshot->onStartSaving(name);
                     return 0;
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
                     snapshot->onStartLoading(name);
                     return 0;
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
                     snapshot->onStartDelete(name);
                     return 0;
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

// TODO: implement optimized SSE2-3-4 version.
bool isBufferZeroed(const void* ptr, int32_t size) {
    for (int32_t i = 0; i < size; ++i) {
        if (((const uint8_t*)ptr)[i] != 0) {
            return false;
        }
    }
    return true;
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

void Snapshotter::prepareForLoading(const char* name) {
    if (mSaver && mSaver->snapshot().name() == name) {
        mSaver.clear();
    }
    mLoader.emplace(name);
    mLoader->prepare();
}

OperationStatus Snapshotter::load(const char* name) {
    mVmOperations.snapshotLoad(name, this, nullptr);
    return mLoader->status();
}

void Snapshotter::prepareForSaving(const char* name) {
    if (mLoader && mLoader->snapshot().name() == name) {
        mLoader.clear();
    }
    mSaver.emplace(name);
    mSaver->prepare();
}

OperationStatus Snapshotter::save(const char* name) {
    mVmOperations.snapshotSave(name, this, nullptr);
    return mSaver->status();
}

void Snapshotter::onStartSaving(const char* name) {
    crashreport::CrashReporter::get()->hangDetector().pause(true);
    mLoader.clear();
    if (!mSaver || isComplete(*mSaver)) {
        mSaver.emplace(name);
    }
}

void Snapshotter::onSavingComplete(const char* name, int res) {
    assert(mSaver && name == mSaver->snapshot().name());
    mSaver->complete(res == 0);
    crashreport::CrashReporter::get()->hangDetector().pause(false);
}

void Snapshotter::onStartLoading(const char* name) {
    crashreport::CrashReporter::get()->hangDetector().pause(true);
    mSaver.clear();
    if (!mLoader || isComplete(*mLoader)) {
        mLoader.emplace(name);
    }
}

void Snapshotter::onLoadingComplete(const char* name, int res) {
    assert(mLoader && name == mLoader->snapshot().name());
    mLoader->complete(res == 0);
    crashreport::CrashReporter::get()->hangDetector().pause(false);
}

void Snapshotter::onStartDelete(const char* name) {
    crashreport::CrashReporter::get()->hangDetector().pause(true);
}

void Snapshotter::onDeletingComplete(const char* name, int res) {
    if (res == 0) {
        if (mSaver && mSaver->snapshot().name() == name) {
            mSaver.clear();
        }
        if (mLoader && mLoader->snapshot().name() == name) {
            mLoader.clear();
        }
        path_delete_dir(Snapshot(name).dataDir().c_str());
    }
    crashreport::CrashReporter::get()->hangDetector().pause(false);
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
