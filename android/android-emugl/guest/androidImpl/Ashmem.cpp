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

#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/SharedMemory.h"
#include "AndroidHostCommon.h"

#include <unordered_map>


using android::base::LazyInstance;
using android::base::SharedMemory;

class SharedMemoryStore {
public:
    SharedMemoryStore() = default;

    int create(const char* name, size_t size) {
        SharedMemory mem(name, size);
        mem.create(0755);
        int fd = mem.getFd();
        if (fd >= 0) {
            mMemories.emplace(fd, std::move(mem));
        }
        return fd;
    }

    void close(int fd) {
        mMemories.erase(fd);
    }

private:
    std::unordered_map<int, SharedMemory> mMemories;
};

static LazyInstance<SharedMemoryStore> sStore = LAZY_INSTANCE_INIT;

extern "C" {

EXPORT int ashmem_valid(int fd) {
    // TODO: distinguish ashmem from others
    return true;
}

EXPORT int ashmem_create_region(const char *name, size_t size) {
    return sStore->create(name, size);
}

// TODO
int ashmem_set_prot_region(int fd, int prot);
int ashmem_pin_region(int fd, size_t offset, size_t len);
int ashmem_unpin_region(int fd, size_t offset, size_t len);
int ashmem_get_size_region(int fd);

}
