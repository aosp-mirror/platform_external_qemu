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
#include "snapshot.pb.h"
#include "snapshot_deps.pb.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshot.h"

#include <unordered_set>

#include <fcntl.h>

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define D(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define D(...) (void)0
#endif

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
            D("previous parent map: %s --- parent of --> %s",
              mSnapshotDepsPb.dependencies(i).parent().c_str(),
              mSnapshotDepsPb.dependencies(i).name().c_str());
            mPrevParentMap[mSnapshotDepsPb.dependencies(i).name()] =
                mSnapshotDepsPb.dependencies(i).parent();
        }
    }

    void savePersistentData() {
        mSnapshotDepsPb.Clear();
        for (const auto it : mNextParentMap) {
            D("Next parent map: %s --- parent of --> %s",
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

        for (auto& s : mInfos) {
            if (!s.parent()) {
                roots.push_back(s.name());
                D("root: %s", s.name().c_str());
            } else {
                const auto p = *(s.parent());
                if (mValidParents.find(p) != mValidParents.end()) {
                    mNextParentMap[s.name()] = p;
                    auto& children = deps[p];
                    children.push_back(s.name());
                    D("edge: %s -- parent of --> %s",
                      p.c_str(), s.name().c_str());
                } else {
                    D("stale edge (stale parent): %s -- parent of --> %s",
                      p.c_str(), s.name().c_str());
                    // Look at mPrevParentMap to patch holes in
                    // the dependency tree.
                    auto parentIt = mPrevParentMap.find(p);
                    while (parentIt != mPrevParentMap.end() &&
                           mValidParents.find(parentIt->second) == mValidParents.end()) {
                        D("parent: %s (still not found)", parentIt->second.c_str());
                        parentIt = mPrevParentMap.find(parentIt->second);
                    }

                    if (parentIt == mPrevParentMap.end() ||
                        mValidParents.find(parentIt->second) == mValidParents.end()) {
                        D("Could not recover new parent. New root: %s",
                          s.name().c_str());
                        // Could not recover a suitable new parent.
                        // This snapshot is now a new root.
                        roots.push_back(s.name());
                    } else {
                        // We can patch the hole in the tree.
                        // Set a new parent for that snapshot.
                        auto& children = deps[parentIt->second];
                        children.push_back(s.name());
                        D("recovered edge: %s -- parent of --> %s",
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

