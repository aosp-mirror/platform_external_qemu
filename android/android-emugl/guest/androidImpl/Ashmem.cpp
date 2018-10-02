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
#include <cutils/ashmem.h>

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/SharedMemory.h"
#include "android/base/Uuid.h"

#include "AndroidHostCommon.h"
#include "Ashmem.h"

#include <atomic>
#include <memory>
#include <sstream>
#include <unordered_map>

#include <errno.h>

using android::base::LazyInstance;
using android::base::SharedMemory;

class SharedMemoryStore {
public:
    SharedMemoryStore() = default;

    int create(const char* name, size_t size) {
        std::unique_ptr<SharedMemory> mem =
            std::make_unique<SharedMemory>(name, size);

        mem->createNoMapping(0755);

        int fd = mem->getFd();
        if (fd >= 0) {
            mMemories[fd] = std::move(mem);
        } else {
            fprintf(stderr, "%s: SharedMemory creation failed! errno: %d\n",
                    __func__, errno);
        }
        return fd;
    }

    void clear() {
        mMemories.clear();
    }

    void close(int fd) {
        mMemories.erase(fd);
    }

    int nextUniqueId() {
        return std::atomic_fetch_add(&mNextId, 1);
    }

private:
    std::atomic<int> mNextId = { 0 };
    std::unordered_map<int, std::unique_ptr<SharedMemory>> mMemories;
};

static LazyInstance<SharedMemoryStore> sStore = LAZY_INSTANCE_INIT;

extern "C" {

EXPORT int ashmem_valid(int fd) {
    // TODO: distinguish ashmem from others
    return true;
}

// ashmem_create_region seems to mean to create a new
// subregion associated with |name|, not necessarily share
// the region named |name|. So we implement it in terms of
// the name + a number counting from 0.
EXPORT int ashmem_create_region(const char *name, size_t size) {
    std::stringstream ss;
    ss << name;
    ss << sStore->nextUniqueId();

    std::string nameWithHash = ss.str();

    return sStore->create(nameWithHash.c_str(), size);
}

EXPORT void ashmem_teardown() {
    sStore->clear();
}

// TODO
int ashmem_set_prot_region(int fd, int prot);
int ashmem_pin_region(int fd, size_t offset, size_t len);
int ashmem_unpin_region(int fd, size_t offset, size_t len);
int ashmem_get_size_region(int fd);

}
