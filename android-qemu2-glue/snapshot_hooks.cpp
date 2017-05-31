// Copyright 2017 The Android Open Source Project
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
// limitations under the License

#include "android-qemu2-glue/snapshot_hooks.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/Optional.h"
#include "android/base/system/System.h"
#include "android/base/Uuid.h"
#include "android/base/StringFormat.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/FunctorThread.h"
#include "android/utils/eintr_wrapper.h"
#include "android/avd/info.h"
#include "android/globals.h"

extern "C" {
#include "qemu/osdep.h"
#include "migration/qemu-file.h"
#include "migration/migration.h"
#include "qemu/cutils.h"
#include "sysemu/sysemu.h"
}

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <linux/userfaultfd.h>

#include <algorithm>
#include <cassert>

using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::StdioStream;
using android::base::System;
using android::base::Optional;
using android::base::FunctorThread;
using android::base::MessageChannel;

static constexpr uint32_t kNoBlock = uint32_t(-1);
static constexpr uint32_t kNewBlockStart = 0x80000000;

static std::string makeRamSaveFilename(const std::string& uuid) {
    auto dir = avdInfo_getContentPath(android_avdInfo);
    auto path =
            PathUtils::join(
                dir, android::base::StringFormat("snapshot_ram_%s.bin", uuid));
    return path;
}

static bool isPageZeroed(const void* ptr, int size) {
    return buffer_is_zero(ptr, size);
}

class RamSaver {
public:
    RamSaver(QEMUFile* f)
        : mUuid(android::base::Uuid::generateFast().toString()),
          mStream(fopen(makeRamSaveFilename(mUuid).c_str(), "wb"), StdioStream::kOwner),
          mThread([this]() { pageHandlingWorker(); }) {
#if SNAPSHOT_PROFILE > 1
        mStartTime = System::get()->getHighResTimeUs();
#endif
        mThread.start();
        assert(mUuid.size() < 128);
        qemu_put_byte(f, mUuid.size());
        qemu_put_buffer(f, (const uint8_t*)mUuid.data(), mUuid.size());

    }

    bool savePage(ram_addr_t blockOffset, const char* blockId,
                  const void* blockHostPtr, ram_addr_t offset, size_t size) {
        assert((offset % size) == 0);
        mQueue.send({blockId, blockHostPtr,
                              (uint32_t)(offset / size), (uint32_t)size});
        return true;
    }

    void done() {
        mQueue.send({});
    }

    ~RamSaver() {
#if SNAPSHOT_PROFILE > 1
        auto start = System::get()->getHighResTimeUs();
#endif

        done();
        mThread.wait();

#if SNAPSHOT_PROFILE > 1
        printf("Async RAM join() time: %.03f\n",
               (System::get()->getHighResTimeUs() - start) / 1000.0);
#endif
    }

    void onVmStateChange(bool running, RunState state) {
        //if (running && state == RUN_STATE_RUNNING && mThread) {
        //    join();
        //}
    }

private:
    struct PageInfo {
        const char* blockId;
        const void* blockHostPtr;
        uint32_t offset;
        uint32_t size;
    };

    struct Block {
        const char* id = nullptr;
        const void* hostPtr = nullptr;
        uint32_t pageSize = 0;
        std::vector<uint32_t> zeroPages;

        struct Page {
            uint32_t offset;
            uint64_t filePos;
        };
        std::vector<Page> nonZeroPages;
    };

    struct Index {
        uint64_t startPos;
        std::vector<Block> blocks;
    };

    void pageHandlingWorker() {
        // Put a placeholder for the index offset.
        mStream.putBe64(0);

        PageInfo pi;
        for (;;) {
            mQueue.receive(&pi);
            if (!handlePageSave(pi)) {
                break;
            }
        }

        fseek(mStream.get(), 0, SEEK_SET);
        mStream.putBe64(mIndex.startPos);

#if SNAPSHOT_PROFILE > 1
        printf("Async RAM saving time: %.03f\n",
               (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
    }

    bool handlePageSave(const PageInfo& pi) {
        if (!pi.blockId) {
            mIndex.startPos = ftell(mStream.get());
            writeIndex();
            return false;
        }

        Block* block;

        if (mIndex.blocks.empty() || pi.blockHostPtr != mIndex.blocks.back().hostPtr) {
            mIndex.blocks.emplace_back();
            block = &mIndex.blocks.back();
            block->hostPtr = pi.blockHostPtr;
            block->pageSize = pi.size;
            block->id = pi.blockId;
        } else {
            block = &mIndex.blocks.back();
        }

        auto ptr = static_cast<const uint8_t*>(block->hostPtr) + pi.offset * pi.size;
        if (isPageZeroed(ptr, pi.size)) {
            block->zeroPages.push_back(pi.offset);
        } else {
            block->nonZeroPages.push_back({pi.offset, (uint64_t)ftell(mStream.get())});
            mStream.write(ptr, pi.size);
        }

        return true;
    }

    void writeIndex() {
        auto start = ftell(mStream.get());
        mStream.putBe32(mIndex.blocks.size());
        for (const Block& b : mIndex.blocks) {
            mStream.putByte(strlen(b.id));
            mStream.write(b.id, strlen(b.id));
            mStream.putBe32(b.pageSize);
            mStream.putBe32(b.zeroPages.size());
            for (auto offset : b.zeroPages) {
                mStream.putBe32(offset);
            }
            mStream.putBe32(b.nonZeroPages.size());
            for (const Block::Page& page : b.nonZeroPages) {
                mStream.putBe32(page.offset);
                mStream.putBe64(page.filePos);
            }
        }
        auto end = ftell(mStream.get());
        printf("index size: %d\n", (int)(end - start));
    }

    std::string mUuid;
    StdioStream mStream;

    FunctorThread mThread;
    MessageChannel<PageInfo, 524288> mQueue;

    Index mIndex;

#if SNAPSHOT_PROFILE > 1
    System::WallDuration mStartTime;
#endif
};

static android::base::LazyInstance<std::unique_ptr<RamSaver>> sRamSaver = {};

static std::string readRamFileUuid(QEMUFile* f) {
    int uuidSize = qemu_get_byte(f);
    std::string uuid(uuidSize, '\0');
    qemu_get_buffer(f, (uint8_t*)&uuid[0], uuidSize);
    return uuid;
}

class RamLoader {
    int userfaultFd;
    int exitFd;

public:
    RamLoader(QEMUFile* f)
        : mUuid(readRamFileUuid(f)),
          mStream(fopen(makeRamSaveFilename(mUuid).c_str(), "rb"), StdioStream::kOwner),
          mReadingThread([this]() { readingWorker(); }) {
#if SNAPSHOT_PROFILE > 1
        mStartTime = System::get()->getHighResTimeUs();
#endif
        readIndex();
        setupProtection();

        //mReadingThread->start();
    }

    void join() {
        mReadingThread.wait();
    }

    ~RamLoader() = default;

    void onVmStateChange(bool running, RunState state) {
        if (running && state == RUN_STATE_RUNNING) {
            //done();
            //mReadingThread.wait();
        }
    }

    void loadPage(void* ptr) {
        Page& page = this->page(ptr);
        if (!page.loaded) {
#if SNAPSHOT_PROFILE > 2
            printf("loading page %p\n", ptr);
#endif

            uint8_t buf[4096];
            assert(pageSize(page) <= (int)sizeof(buf));
            iovec vec = { buf, (size_t)pageSize(page) };
            ssize_t read = (ssize_t)HANDLE_EINTR(preadv(mFd, &vec, 1, page.filePos));
            if (read != (ssize_t)vec.iov_len) {
                printf("WTF: %d %d %p %d\n", (int)read, (int)vec.iov_len, vec.iov_base, errno);
            }

            struct uffdio_copy copy_struct;

            copy_struct.dst = (uint64_t)(uintptr_t)pagePtr(page);
            copy_struct.src = (uint64_t)(uintptr_t)&buf[0];
            copy_struct.len = pageSize(page);
            copy_struct.mode = 0;

            /* copy also acks to the kernel waking the stalled thread up
             * TODO: We can inhibit that ack and only do it if it was requested
             * which would be slightly cheaper, but we'd have to be careful
             * of the order of updating our page state.
             */
            if (ioctl(userfaultFd, UFFDIO_COPY, &copy_struct)) {
                int e = errno;
                printf("%s: %s copy host: %p from: %p",
                             __func__, strerror(e), (void*)copy_struct.dst,
                       (void*)copy_struct.src);

                return;
            }

            page.loaded = true;

            struct uffdio_range range_struct;
            range_struct.start = (uintptr_t)pagePtr(page);
            range_struct.len = pageSize(page);

            if (ioctl(userfaultFd, UFFDIO_UNREGISTER, &range_struct)) {
                printf("%s: userfault unregister %s", __func__, strerror(errno));
            }
        }
    }

private:
    struct Page {
        uint16_t blockIndex;
        bool loaded;
        uint32_t offset;
        uint64_t filePos;
    };

    struct Block {
        uint32_t pageSize;
        RAMBlock* ramBlock;

        uint8_t* hostPtr() const { return (uint8_t*)qemu_host_from_ram_block(ramBlock); }
    };

    struct Index {
        std::vector<Block> blocks;
        std::vector<Page> pages;
    };

    void readIndex() {
#if SNAPSHOT_PROFILE > 1
        auto start = System::get()->getHighResTimeUs();
#endif

        mFd = fileno(mStream.get());
        uint64_t indexPos = mStream.getBe64();
        fseek(mStream.get(), indexPos, SEEK_SET);

        int blockCount = mStream.getBe32();
        mIndex.blocks.reserve(blockCount);
        for (int i = 0; i != blockCount; ++i) {
            mIndex.blocks.emplace_back();
            Block& block = mIndex.blocks.back();

            int nameLen = mStream.getByte();
            char name[256];
            mStream.read(name, nameLen);
            name[nameLen] = '\0';

            block.ramBlock = qemu_ram_block_by_name(name);
            assert(block.ramBlock);

            block.pageSize = mStream.getBe32();

            uint32_t zeroPagesCount = mStream.getBe32();
            for (uint32_t j = 0; j != zeroPagesCount; ++j) {
                uint32_t offset = mStream.getBe32();
                auto ptr = block.hostPtr() + (uint64_t)offset * block.pageSize;
                if (!isPageZeroed(ptr, block.pageSize)) {
                    memset(ptr, 0, block.pageSize);
                }
            }

            uint32_t blockPagesCount = mStream.getBe32();
            for (uint32_t j = 0; j != blockPagesCount; ++j) {
                mIndex.pages.emplace_back();
                Page& page = mIndex.pages.back();
                page.blockIndex = i;
                page.loaded = false;
                page.offset = mStream.getBe32();
                page.filePos = mStream.getBe64();
            }
        }

#if SNAPSHOT_PROFILE > 1
        printf("readIndex() time: %.03f\n",
               (System::get()->getHighResTimeUs() - start) / 1000.0);
#endif
    }

    uint8_t* pagePtr(const Page& page) const {
        const Block& block = mIndex.blocks[page.blockIndex];
        return block.hostPtr() + (uint64_t)page.offset * block.pageSize;
    }

    int pageSize(const Page& page) const {
        return mIndex.blocks[page.blockIndex].pageSize;
    }

    Page& page(void* ptr) {
        auto it = std::upper_bound(
                    mIndex.pages.begin(), mIndex.pages.end(), ptr,
                    [this](const void* ptr, const Page& page) {
                        return ptr < pagePtr(page) + pageSize(page);
                    });
        assert(it != mIndex.pages.end());
        assert(ptr >= pagePtr(*it));
        assert(ptr < pagePtr(*it) + pageSize(*it));
        return *it;
    }

    void setupProtection();

    void readingWorker() {
        for (;;) {
            struct pollfd pfd[2];

            /*
             * We're mainly waiting for the kernel to give us a faulting HVA,
             * however we can be told to quit via userfault_quit_fd which is
             * an eventfd
             */
            pfd[0].fd = userfaultFd;
            pfd[0].events = POLLIN;
            pfd[0].revents = 0;
            pfd[1].fd = exitFd;
            pfd[1].events = POLLIN; /* Waiting for eventfd to go positive */
            pfd[1].revents = 0;

            if (poll(pfd, 2, -1 /* Wait forever */) == -1) {
                printf("%s: userfault poll: %s", __func__, strerror(errno));
                break;
            }

            if (pfd[1].revents) {
                break;
            }

            uffd_msg msg;
            int ret = read(userfaultFd, &msg, sizeof(msg));
            if (ret != sizeof(msg)) {
                if (errno == EAGAIN) {
                    /*
                     * if a wake up happens on the other thread just after
                     * the poll, there is nothing to read.
                     */
                    continue;
                }
                if (ret < 0) {
                    printf("%s: Failed to read full userfault message: %s",
                                 __func__, strerror(errno));
                    break;
                } else {
                    printf("%s: Read %d bytes from userfaultfd expected %zd",
                                 __func__, ret, sizeof(msg));
                    break; /* Lost alignment, don't know what we'd read next */
                }
            }
            if (msg.event != UFFD_EVENT_PAGEFAULT) {
                printf("%s: Read unexpected event %ud from userfaultfd",
                             __func__, msg.event);
                continue; /* It's not a page fault, shouldn't happen */
            }

            auto ptr = (void *)(uintptr_t)msg.arg.pagefault.address;
            loadPage(ptr);
        }
#if SNAPSHOT_PROFILE > 1
        printf("Async RAM loading time: %.03f\n",
               (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
    }

    std::string mUuid;
    StdioStream mStream;
    int mFd;
    FunctorThread mReadingThread;

    Index mIndex;
    struct sigaction mOldAction;

#if SNAPSHOT_PROFILE > 1
    System::WallDuration mStartTime;
#endif
};

static android::base::LazyInstance<std::unique_ptr<RamLoader>> sRamLoader = {};


void RamLoader::setupProtection() {
#if SNAPSHOT_PROFILE > 1
    auto start = System::get()->getHighResTimeUs();
#endif

    userfaultFd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    assert(userfaultFd >= 0);

    uffdio_api api_struct;
    uint64_t ioctl_mask;

    api_struct.api = UFFD_API;
    api_struct.features = 0;
    if (ioctl(userfaultFd, UFFDIO_API, &api_struct)) {
        printf("UFFDIO_API failed: %s", strerror(errno));
        return;
    }

    ioctl_mask = (__u64)1 << _UFFDIO_REGISTER |
                 (__u64)1 << _UFFDIO_UNREGISTER;
    if ((api_struct.ioctls & ioctl_mask) != ioctl_mask) {
        printf("Missing userfault features: %" PRIx64,
                     (uint64_t)(~api_struct.ioctls & ioctl_mask));
        return;
    }

    exitFd = eventfd(0, EFD_CLOEXEC);

    mReadingThread.start();

    uint8_t* startPtr = nullptr;
    uint32_t curSize = 0;
    for (const Page& page : mIndex.pages) {
        auto ptr = pagePtr(page);
        auto size = pageSize(page);
        if (ptr == startPtr + curSize) {
            curSize += size;
        } else {
            if (startPtr) {
                madvise(startPtr, curSize, MADV_DONTNEED);
                struct uffdio_register reg_struct;

                reg_struct.range.start = (uintptr_t)startPtr;
                reg_struct.range.len = curSize;
                reg_struct.mode = UFFDIO_REGISTER_MODE_MISSING;

                /* Now tell our userfault_fd that it's responsible for this area */
                if (ioctl(userfaultFd, UFFDIO_REGISTER, &reg_struct)) {
                    printf("%s userfault register: %s", __func__, strerror(errno));
                    return;
                }
            }
            startPtr = ptr;
            curSize = size;
        }
    }
    if (startPtr) {
        madvise(startPtr, curSize, MADV_DONTNEED);
        struct uffdio_register reg_struct;

        reg_struct.range.start = (uintptr_t)startPtr;
        reg_struct.range.len = curSize;
        reg_struct.mode = UFFDIO_REGISTER_MODE_MISSING;

        /* Now tell our userfault_fd that it's responsible for this area */
        if (ioctl(userfaultFd, UFFDIO_REGISTER, &reg_struct)) {
            printf("%s userfault register: %s", __func__, strerror(errno));
            return;
        }
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = [](int sig, siginfo_t *si, void *unused) { sRamLoader->get()->loadPage(si->si_addr); };
    ::sigaction(SIGSEGV, &sa, &mOldAction);

#if SNAPSHOT_PROFILE > 1
    printf("mprotect() time: %.03f\n",
           (System::get()->getHighResTimeUs() - start) / 1000.0);
#endif
}


static QEMUFileHooks sSaveHooks = {
    .before_ram_iterate = [](QEMUFile *f, void *opaque, uint64_t flags, void *data) {
        qemu_put_be64(f, RAM_SAVE_FLAG_HOOK);
        if (flags == RAM_CONTROL_SETUP) {
            sRamSaver->reset(new RamSaver(f));
        }
        return 0;
    },
    .after_ram_iterate = [](QEMUFile *f, void *opaque, uint64_t flags, void *data) {
        if (flags == RAM_CONTROL_FINISH) {
            sRamSaver->reset();
        }
        return 0;
    },
    .hook_ram_load = nullptr,
    .save_page = nullptr,
    .save_page_ex =  [](QEMUFile *f, void *opaque, ram_addr_t block_offset, const char* block_id, const void* block_host_ptr, ram_addr_t offset, size_t size, uint64_t *bytes_sent) -> size_t {
        sRamSaver.get()->savePage(block_offset, block_id, block_host_ptr, offset, size);
        *bytes_sent = 1;
        return RAM_SAVE_CONTROL_DELAYED;
    },
};

static QEMUFileHooks sLoadHooks = {
    .before_ram_iterate = nullptr,
    .after_ram_iterate = nullptr,
    .hook_ram_load = [](QEMUFile *f, void *opaque, uint64_t flags, void *data) {
        printf("hook_ram_load: 0x%llu\n", (unsigned long long)flags);
        static int counter = 0;
        if (flags == RAM_CONTROL_HOOK) {
            if (++counter == 1) {
                sRamLoader->reset(new RamLoader(f));
            }
        }
        return 0;
    }
};

void qemu_snapshot_hooks_setup() {
    migrate_set_file_hooks(&sSaveHooks, &sLoadHooks);

    qemu_add_vm_change_state_handler([](void* opaque, int running, RunState state) {
        if (auto rs = sRamSaver->get()) {
            rs->onVmStateChange(running != 0, state);
        }
        if (auto rl = sRamLoader->get()) {
            rl->onVmStateChange(running != 0, state);
        }
    }, nullptr);
}
