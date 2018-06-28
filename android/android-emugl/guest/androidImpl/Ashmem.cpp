#include <cutils/ashmem.h>

#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/SharedMemory.h"

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

int ashmem_valid(int fd) {
    // TODO: distinguish ashmem from others
    return true;
}

int ashmem_create_region(const char *name, size_t size) {
    return sStore->create(name, size);
}

// TODO
int ashmem_set_prot_region(int fd, int prot);
int ashmem_pin_region(int fd, size_t offset, size_t len);
int ashmem_unpin_region(int fd, size_t offset, size_t len);
int ashmem_get_size_region(int fd);

}
