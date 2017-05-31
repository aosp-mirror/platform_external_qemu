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
using android::base::Stream;
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

void writeSmallInt(Stream& stream, uint64_t val) {
    assert(uint32_t(val) == val);
    do {
        auto byte = uint8_t(val & 0x7f);
        val >>= 7;
        if (val) {
            byte |= 0x80;
        }
        stream.putByte(byte);
    } while (val != 0);
}

uint32_t readSmallInt(Stream& stream) {
    uint32_t res = 0;
    uint8_t byte;
    int i = 0;
    do {
        byte = stream.getByte();
        assert((res & 0xfe000000) == 0);
        res |= (byte & 0x7f) << (i++ * 7);
    } while (byte & 0x80);
    return res;
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
        uint32_t totalNonzeroPages;
        std::vector<Block> blocks;
    };

    void pageHandlingWorker() {
        mIndex.totalNonzeroPages = 0;

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
            ++mIndex.totalNonzeroPages;
            block->nonZeroPages.push_back({pi.offset, (uint64_t)ftell(mStream.get())});
            mStream.write(ptr, pi.size);
        }

        return true;
    }

    void writeIndex() {
        auto start = ftell(mStream.get());
        mStream.putBe32(mIndex.totalNonzeroPages);
        mStream.putBe32(mIndex.blocks.size());
        for (const Block& b : mIndex.blocks) {
            const auto len = strlen(b.id);
            mStream.putByte(len);
            mStream.write(b.id, len);
            mStream.putBe32(b.pageSize);
            mStream.putBe32(b.zeroPages.size());
            if (!b.zeroPages.empty()) {
                auto it = b.zeroPages.begin();
                auto offset = *it++;
                mStream.putBe32(offset);
                for (; it != b.zeroPages.end(); ++it) {
                    const auto nextOffset = *it;
                    assert(nextOffset > offset);
                    writeSmallInt(mStream, nextOffset - offset);
                    offset = nextOffset;
                }
            }
            mStream.putBe32(b.nonZeroPages.size());
            if (!b.nonZeroPages.empty()) {
                auto it = b.nonZeroPages.begin();
                const Block::Page* page = &*it++;
                mStream.putBe32(page->offset);
                mStream.putBe64(page->filePos);
                for (; it != b.nonZeroPages.end(); ++it) {
                    const Block::Page& nextPage = *it;
                    assert(nextPage.offset > page->offset);
                    writeSmallInt(mStream, nextPage.offset - page->offset);
                    assert(nextPage.filePos > page->filePos);
                    assert(((nextPage.filePos - page->filePos) % b.pageSize) == 0);
                    writeSmallInt(mStream, (nextPage.filePos - page->filePos) / b.pageSize);
                    page = &nextPage;
                }
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

    struct Page;

public:
    RamLoader(QEMUFile* f)
        : mUuid(readRamFileUuid(f)),
          mStream(fopen(makeRamSaveFilename(mUuid).c_str(), "rb"), StdioStream::kOwner),
          mPagefaultThread([this]() { pagefaultWorker(); }),
          mReaderThread([this]() { readerWorker(); }) {
#if SNAPSHOT_PROFILE > 1
        mStartTime = System::get()->getHighResTimeUs();
#endif
        readIndex();
        setupProtection();
    }

    void join() {
        mPagefaultThread.wait();
    }

    ~RamLoader() = default;

    void onVmStateChange(bool running, RunState state) {
        if (running && state == RUN_STATE_RUNNING) {
            //done();
            //mReadingThread.wait();
        }
    }

    template <class DataDeleter>
    void loadPage(Page* pagePtr, DataDeleter deleter) {
        Page& page = *pagePtr;
        auto state = uint8_t(State::Read);
        if (!page.state.compare_exchange_strong(state, uint8_t(State::Copying))) {
            assert(state == uint8_t(State::Loaded));
            return;
        }

#if SNAPSHOT_PROFILE > 2
        printf("loading page %p\n", this->pagePtr(page));
#endif
        uffdio_copy copy_struct;

        copy_struct.dst = (uint64_t)(uintptr_t)this->pagePtr(page);
        copy_struct.src = (uint64_t)(uintptr_t)page.data;
        copy_struct.len = pageSize(page);
        copy_struct.mode = 0;

        /* copy also acks to the kernel waking the stalled thread up
         * TODO: We can inhibit that ack and only do it if it was requested
         * which would be slightly cheaper, but we'd have to be careful
         * of the order of updating our page state.
         */
        if (ioctl(userfaultFd, UFFDIO_COPY, &copy_struct)) {
            int e = errno;
            printf("%s: %s copy host: %p from: %p\n",
                         __func__, strerror(e), (void*)copy_struct.dst,
                   (void*)copy_struct.src);
            deleter(page.data);
            page.data = nullptr;
            page.state.store(uint8_t(State::Error));
            return;
        }

        deleter(page.data);
        page.data = nullptr;

        uffdio_range range_struct;
        range_struct.start = (uintptr_t)this->pagePtr(page);
        range_struct.len = pageSize(page);
        if (ioctl(userfaultFd, UFFDIO_UNREGISTER, &range_struct)) {
            printf("%s: userfault unregister %s", __func__, strerror(errno));
            page.state.store(uint8_t(State::Error));
            return;
        }

        page.state.store(uint8_t(State::Loaded));
    }

private:
    enum class State : uint8_t {
        Clean,
        Reading,
        Read,
        Copying,
        Loaded,
        Error
    };

    struct Page {
        std::atomic<uint8_t> state;
        uint16_t blockIndex;
        uint32_t offset;
        uint64_t filePos;
        uint8_t* data;

        Page() = default;
        Page(Page&& other) {
            state = other.state.load();
            blockIndex = other.blockIndex;
            offset = other.offset;
            filePos = other.filePos;
            data = other.data;
        }
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

        int nonzeroPageCount = mStream.getBe32();
        mIndex.pages.reserve(nonzeroPageCount);
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
            if (uint32_t zeroPagesCount = mStream.getBe32()) {
                uint32_t offset = mStream.getBe32();
                auto ptr = block.hostPtr() + (uint64_t)offset * block.pageSize;
                if (!isPageZeroed(ptr, block.pageSize)) {
                    memset(ptr, 0, block.pageSize);
                }
                for (uint32_t j = 1; j != zeroPagesCount; ++j) {
                    auto diff = readSmallInt(mStream);
                    offset += diff;
                    auto ptr = block.hostPtr() + (uint64_t)offset * block.pageSize;
                    if (!isPageZeroed(ptr, block.pageSize)) {
                        memset(ptr, 0, block.pageSize);
                    }
                }
            }

            if (uint32_t blockPagesCount = mStream.getBe32()) {
                mIndex.pages.emplace_back();
                Page& page = mIndex.pages.back();
                page.blockIndex = i;
                page.state = uint8_t(State::Clean);
                uint32_t offset = page.offset = mStream.getBe32();
                uint64_t pos = page.filePos = mStream.getBe64();

                for (uint32_t j = 1; j != blockPagesCount; ++j) {
                    const auto offsetDiff = readSmallInt(mStream);
                    const auto posDiff = readSmallInt(mStream);
                    mIndex.pages.emplace_back();
                    Page& page = mIndex.pages.back();
                    page.blockIndex = i;
                    page.state = uint8_t(State::Clean);

                    offset += offsetDiff;
                    pos += posDiff * block.pageSize;
                    page.offset = offset;
                    page.filePos = pos;
                }
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
        if (ptr < pagePtr(*it)) {
            printf("WTF1: %p %p\n", ptr, pagePtr(*it));
            assert(0);
        }
        assert(ptr < pagePtr(*it) + pageSize(*it));
        return *it;
    }

    void setupProtection();

    template <class BufferGetter>
    bool readData(Page* pagePtr, BufferGetter bufGetter) {
        Page& page = *pagePtr;
        auto state = uint8_t(State::Clean);
        if (!page.state.compare_exchange_strong(state, (uint8_t)State::Reading)) {
            // Spin until the reading thread finishes.
            while (state < uint8_t(State::Read)) {
                state = uint8_t(page.state.load());
            }
            return false;
        }

        uint8_t* buf = bufGetter(pageSize(page));
        iovec vec = { buf, (size_t)pageSize(page) };
        ssize_t read = (ssize_t)HANDLE_EINTR(preadv(mFd, &vec, 1, page.filePos));
        if (read != (ssize_t)vec.iov_len) {
            printf("WTF: %d %d %p %d\n", (int)read, (int)vec.iov_len, vec.iov_base, errno);
            page.state.store(uint8_t(State::Error));
            return false;
        }

        page.data = buf;
        page.state.store(uint8_t(State::Read));
        return true;
    }

    void readerWorker() {
        while (auto pagePtr = mReadingQueue.receive()) {
            Page* page = *pagePtr;
            if (!page) {
                mReadDataQueue.send(nullptr);
                mReadingQueue.stop();
                break;
            }

            if (readData(page, [](int size) { return new uint8_t[size]; })) {
                mReadDataQueue.send(page);
            }
        }
    }

    void* readNextPagefault() const {
        uffd_msg msg;
        const int ret = HANDLE_EINTR(read(userfaultFd, &msg, sizeof(msg)));
        if (ret != sizeof(msg)) {
            if (errno == EAGAIN) {
                /*
                 * if a wake up happens on the other thread just after
                 * the poll, there is nothing to read.
                 */
                return nullptr;
            }
            if (ret < 0) {
                printf("%s: Failed to read full userfault message: %s",
                             __func__, strerror(errno));
                return nullptr;
            } else {
                printf("%s: Read %d bytes from userfaultfd expected %zd",
                             __func__, ret, sizeof(msg));
                return nullptr; /* Lost alignment, don't know what we'd read next */
            }
        }
        if (msg.event != UFFD_EVENT_PAGEFAULT) {
            printf("%s: Read unexpected event %ud from userfaultfd",
                         __func__, msg.event);
            return nullptr; /* It's not a page fault, shouldn't happen */
        }

        return (void *)(uintptr_t)msg.arg.pagefault.address;
    }

    void pagefaultWorker() {
        auto it = mIndex.pages.begin();
        for (;;) {
            pollfd pfd[2];

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

            timespec timeout = { 0, 1000 };
            if (mReadingQueue.size() == mReadingQueue.capacity() ||
                it == mIndex.pages.end()) {
                timeout.tv_nsec = 500000;
            }
            if (ppoll(pfd, 2, &timeout, nullptr) == -1) {
                printf("%s: userfault poll: %s", __func__, strerror(errno));
                break;
            }

            if (pfd[1].revents) {
                break;
            }

            Page* page;
            if (pfd[0].revents) {
                while (auto ptr = readNextPagefault()) {
                    Page& page = this->page(ptr);
                    uint8_t buf[4096];
                    readData(&page, [&buf](int) { return buf; });
                    assert(page.state >= (uint8_t)State::Read);
                    loadPage(&page, [&buf](uint8_t* ptr) {
                        if (ptr != &buf[0]) {
                            delete[] ptr;
                        }
                    });
                }
            } else if (mReadDataQueue.tryReceive(&page)) {
                if (!page) {
                    mReadDataQueue.stop();
                } else {
                    loadPage(page, [](uint8_t* ptr){ delete [] ptr; });
                }
            } else {
                if (mReadingQueue.isStopped() && mReadDataQueue.isStopped()) {
                    break;
                }

                // Find next page to queue
                it = std::find_if(it, mIndex.pages.end(), [](const Page& page) {
                    return page.state.load() == uint8_t(State::Clean);
                });
                if (it != mIndex.pages.end()) {
                    if (mReadingQueue.trySend(&*it)) {
                        ++it;
                    }
                } else {
                    mReadingQueue.trySend(nullptr);
                }
            }
        }

        mReadingQueue.stop();
        mReadDataQueue.stop();
        mReaderThread.wait();

#if SNAPSHOT_PROFILE > 1
        printf("Async RAM loading time: %.03f\n",
               (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
    }

    std::string mUuid;
    StdioStream mStream;
    int mFd;
    FunctorThread mPagefaultThread;
    FunctorThread mReaderThread;

    MessageChannel<Page*, 32> mReadingQueue;
    MessageChannel<Page*, 32> mReadDataQueue;

    Index mIndex;

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

    mPagefaultThread.start();
    mReaderThread.start();

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
