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

#include "android/snapshot/Hierarchy.h"

#include "android/base/files/ScopedFd.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/globals.h"
#include "android/protobuf/LoadSave.h"
#include "android/snapshot/common.h"
#include "android/snapshot/proto/snapshot.pb.h"
#include "android/snapshot/proto/snapshot_deps.pb.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshot.h"

#include <unordered_set>

#include <fcntl.h>

using android::base::LazyInstance;
using android::protobuf::loadProtobuf;
using android::protobuf::ProtobufLoadResult;
using android::protobuf::saveProtobuf;
using android::protobuf::ProtobufSaveResult;

namespace pb = emulator_snapshot;

namespace android {
namespace snapshot {

class HierarchyImpl {
public:
    HierarchyImpl() { }

    void loadPersistentData() {
        mPrevParentMap.clear();
        mNextParentMap.clear();
        auto loadRes =
            loadProtobuf(
                getSnapshotDepsFileName(),
                mSnapshotDepsPb);

        if (loadRes != ProtobufLoadResult::Success) return;

        for (int i = 0; i < mSnapshotDepsPb.dependencies_size(); i++) {
            fprintf(stderr, "%s: previous parent map: %s --- parent of --> %s\n" ,__func__,
                    mSnapshotDepsPb.dependencies(i).parent().c_str(),
                    mSnapshotDepsPb.dependencies(i).name().c_str());
            mPrevParentMap[mSnapshotDepsPb.dependencies(i).name()] =
                mSnapshotDepsPb.dependencies(i).parent();
        }
    }

    void savePersistentData() {
        mSnapshotDepsPb.Clear();
        for (const auto it : mNextParentMap) {
            fprintf(stderr, "%s: Next parent map: %s --- parent of --> %s\n" ,__func__,
                    it.second.c_str(),
                    it.first.c_str());
            auto dep = mSnapshotDepsPb.add_dependencies();
            dep->set_name(it.first);
            dep->set_parent(it.second);
        }
        saveProtobuf(
            getSnapshotDepsFileName(),
            mSnapshotDepsPb);
        mNextParentMap.clear();
    }

    void constructDeps() {
        loadPersistentData();
        mInfos = Snapshot::getExistingSnapshots();

        auto& roots = depInfo.roots;
        auto& deps = depInfo.deps;

        roots.clear();
        deps.clear();

        std::vector<std::string> todo = {};

        for (const auto& s : mInfos) {
            mValidParents.insert(s.name());
        }

        for (const auto& s : mInfos) {
            if (!s.parent()) {
                roots.push_back(s.name());
                fprintf(stderr, "%s: root: %s\n", __func__, s.name().c_str());
            } else {
                const auto& p = *(s.parent());
                if (mValidParents.find(p) != mValidParents.end()) {
                    mNextParentMap[s.name()] = p;
                    auto& children = deps[p];
                    children.push_back(s.name());
                    fprintf(stderr, "%s: edge: %s -- parent of --> %s\n", __func__,
                            p.c_str(), s.name().c_str());
                } else {
                    fprintf(stderr, "%s: stale edge (stale parent): %s -- parent of --> %s\n", __func__,
                            p.c_str(), s.name().c_str());
                    // Look at mPrevParentMap to patch holes in
                    // the dependency tree.
                    auto parentIt = mPrevParentMap.find(p);
                    while (parentIt != mPrevParentMap.end() &&
                           mValidParents.find(parentIt->second) == mValidParents.end()) {
                        fprintf(stderr, "%s: parent: %s (still not found)\n", __func__, parentIt->second.c_str());
                        parentIt = mPrevParentMap.find(parentIt->second);
                    }

                    if (parentIt == mPrevParentMap.end() ||
                        mValidParents.find(parentIt->second) == mValidParents.end()) {
                        fprintf(stderr, "%s: Could not recover new parent. New root: %s\n", __func__, s.name().c_str());
                        // Could not recover a suitable new parent.
                        // This snapshot is now a new root.
                        roots.push_back(s.name());
                    } else {
                        // Set a new parent for that snapshot.
                        auto& children = deps[parentIt->second];
                        children.push_back(s.name());
                        fprintf(stderr, "%s: recovered edge: %s -- parent of --> %s\n", __func__,
                                parentIt->second.c_str(), s.name().c_str());
                        mNextParentMap[s.name()] = parentIt->second;
                    }
                }
            }
        }
        savePersistentData();
    }

    Hierarchy::Info depInfo;

private:
    std::vector<Snapshot> mInfos;
    std::unordered_set<std::string> mValidParents;

    pb::SnapshotDependencies mSnapshotDepsPb;
    std::map<std::string, std::string> mPrevParentMap;
    std::map<std::string, std::string> mNextParentMap;
};

static LazyInstance<HierarchyImpl> sHierarchyImpl;

Hierarchy::Hierarchy() {
    sHierarchyImpl.get();
}

static LazyInstance<Hierarchy> sProvenance;

// static
Hierarchy* Hierarchy::get() {
    return sProvenance.ptr();
}

const Hierarchy::Info& Hierarchy::currentInfo() const {
    sHierarchyImpl->constructDeps();
    return sHierarchyImpl->depInfo;
}

}  // namespace snapshot
}  // namespace android

