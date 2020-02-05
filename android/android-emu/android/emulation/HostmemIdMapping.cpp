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
#include "android/emulation/HostmemIdMapping.h"
#include "android/base/memory/LazyInstance.h"

using android::base::LazyInstance;

namespace android {
namespace emulation {

// static
const HostmemIdMapping::Id HostmemIdMapping::kInvalidHostmemId = 0;

static LazyInstance<HostmemIdMapping> sMapping = LAZY_INSTANCE_INIT;

// static
HostmemIdMapping* HostmemIdMapping::get() {
    return sMapping.ptr();
}

// TODO: Add registerHostmemFixed version that takes a predetermined id,
// for snapshots
HostmemIdMapping::Id HostmemIdMapping::add(uint64_t hva, uint64_t size) {
    if (0 == hva || 0 == size) return kInvalidHostmemId;

    Id wantedId = mCurrentId++;
    HostmemIdMapping::Entry entry;
    entry.id = wantedId;
    entry.hva = hva;
    entry.size = size;
    mEntries.set(wantedId, entry);
    return wantedId;
}

void HostmemIdMapping::remove(HostmemIdMapping::Id id) {
    mEntries.erase(id);
}

HostmemIdMapping::Entry HostmemIdMapping::get(HostmemIdMapping::Id id) const {
    const HostmemIdMapping::Entry badEntry {
        kInvalidHostmemId, 0, 0,
    };

    if (kInvalidHostmemId == id) return badEntry;

    auto entry = mEntries.get(id);

    if (!entry) return badEntry;

    return *entry;
}

void HostmemIdMapping::clear() {
    mEntries.clear();
}

} // namespace android
} // namespace emulation

// C interface for use with vm operations
extern "C" {

uint64_t android_emulation_hostmem_register(uint64_t hva, uint64_t size) {
    return static_cast<uint64_t>(
        android::emulation::HostmemIdMapping::get()->add(hva, size));
}

void android_emulation_hostmem_unregister(uint64_t id) {
    android::emulation::HostmemIdMapping::get()->remove(id);
}

HostmemEntry android_emulation_hostmem_get_info(uint64_t id) {
    return static_cast<HostmemEntry>(
        android::emulation::HostmemIdMapping::get()->get(id));
}

} // extern "C"
