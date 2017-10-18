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

#include "android/snapshot/Provenance.h"

#include "android/base/memory/LazyInstance.h"
#include "android/globals.h"
#include "android/snapshot/common.h"
#include "android/snapshot/proto/snapshot.pb.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshot.h"

using android::base::LazyInstance;

namespace android {
namespace snapshot {

class ProvenanceImpl {
public:
    ProvenanceImpl() { }

    void constructDeps() {
        snapshotInfos = Snapshot::getExistingSnapshots();

        auto& roots = depInfo.roots;
        auto& deps = depInfo.deps;

        roots.clear();
        deps.clear();

        std::vector<std::string> todo = {};

        for (const auto& s : snapshotInfos) {
            if (!s.parent()) {
                roots.push_back(s.name());
                fprintf(stderr, "%s: root: %s\n", __func__, s.name().c_str());
            } else {
                const auto& p = *(s.parent());
                auto& children = deps[p];
                children.push_back(s.name());
                fprintf(stderr, "%s: edge: %s -- parent of --> %s\n", __func__,
                        p.c_str(), s.name().c_str());
            }
        }
    }

    std::vector<Snapshot> snapshotInfos;
    Provenance::DependencyInfo depInfo;
};

static LazyInstance<ProvenanceImpl> sProvenanceImpl;

Provenance::Provenance() {
    sProvenanceImpl.get();
}

static LazyInstance<Provenance> sProvenance;

// static
Provenance* Provenance::get() {
    return sProvenance.ptr();
}

const Provenance::DependencyInfo& Provenance::updateAndGetDependencies() const {
    sProvenanceImpl->constructDeps();
    return sProvenanceImpl->depInfo;
}

}  // namespace snapshot
}  // namespace android

