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

#include "android/avd/info.h"
#include "android/base/Optional.h"
#include "android/base/StringFormat.h"
#include "android/base/Uuid.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/globals.h"
#include "android/utils/eintr_wrapper.h"

extern "C" {
#include "qemu/osdep.h"
}

extern "C" {
#include "migration/migration.h"
#include "migration/qemu-file.h"
#include "qemu/cutils.h"
#include "sysemu/sysemu.h"
}

#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using android::base::LazyInstance;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::ScopedFd;
using android::base::StdioStream;
using android::base::Stream;
using android::base::System;
using android::base::Optional;
using android::base::FunctorThread;
using android::base::MessageChannel;

static std::string makeRamSaveFilename(const std::string& uuid) {
    auto dir = avdInfo_getContentPath(android_avdInfo);
    auto path = PathUtils::join(
            dir, android::base::StringFormat("snapshot_ram_%s.bin", uuid));
    return path;
}

static bool isPageZeroed(const void* ptr, int size) {
    return buffer_is_zero(ptr, size);
}

namespace {
class OnExitFunctorThread : public android::base::Thread {
public:
    using Main = std::function<void()>;
    using OnExit = void (*)();

    OnExitFunctorThread(Main&& mainFunc, OnExit onExitFunc)
        : mMainFunc(std::move(mainFunc)), mOnExitFunc(onExitFunc) {}

private:
    intptr_t main() override {
        mMainFunc();
        return 0;
    }
    void onExit() override { mOnExitFunc(); }

    Main mMainFunc;
    OnExit mOnExitFunc;
};
}  // namespace

class RamSaver {
public:
    RamSaver(QEMUFile* f)
        : mUuid(android::base::Uuid::generateFast().toString()),
          mStream(fopen(makeRamSaveFilename(mUuid).c_str(), "wb"),
                  StdioStream::kOwner),
          mThread([this]() { pageHandlingWorker(); }) {
#if SNAPSHOT_PROFILE > 1
        mStartTime = System::get()->getHighResTimeUs();
#endif
        mThread.start();
        assert(mUuid.size() < 128);
        qemu_put_byte(f, mUuid.size());
        qemu_put_buffer(f, (const uint8_t*)mUuid.data(), mUuid.size());
    }

    bool savePage(ram_addr_t blockOffset,
                  const char* blockId,
                  const void* blockHostPtr,
                  ram_addr_t offset,
                  size_t size) {
        assert((offset % size) == 0);
        mQueue.send({blockId, blockHostPtr, (uint32_t)(offset / size),
                     (uint32_t)size});
        return true;
    }

    void done() { mQueue.send({}); }

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
        // if (running && state == RUN_STATE_RUNNING && mThread) {
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

        if (mIndex.blocks.empty() ||
            pi.blockHostPtr != mIndex.blocks.back().hostPtr) {
            mIndex.blocks.emplace_back();
            block = &mIndex.blocks.back();
            block->hostPtr = pi.blockHostPtr;
            block->pageSize = pi.size;
            block->id = pi.blockId;
        } else {
            block = &mIndex.blocks.back();
        }

        auto ptr = static_cast<const uint8_t*>(block->hostPtr) +
                   pi.offset * pi.size;
        if (isPageZeroed(ptr, pi.size)) {
            block->zeroPages.push_back(pi.offset);
        } else {
            ++mIndex.totalNonzeroPages;
            block->nonZeroPages.push_back(
                    {pi.offset, (uint64_t)ftell(mStream.get())});
            mStream.write(ptr, pi.size);
        }

        return true;
    }

    void writeIndex() {
        auto start = ftell(mStream.get());
        mStream.putBe32(mIndex.totalNonzeroPages);
        for (const Block& b : mIndex.blocks) {
            mStream.putBe32(b.zeroPages.size());
            if (!b.zeroPages.empty()) {
                auto it = b.zeroPages.begin();
                auto offset = *it++;
                mStream.putBe32(offset);
                for (; it != b.zeroPages.end(); ++it) {
                    const auto nextOffset = *it;
                    assert(nextOffset > offset);
                    mStream.putPackedNum(nextOffset - offset);
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
                    mStream.putPackedNum(nextPage.offset - page->offset);
                    assert(nextPage.filePos > page->filePos);
                    assert(((nextPage.filePos - page->filePos) % b.pageSize) ==
                           0);
                    mStream.putPackedNum((nextPage.filePos - page->filePos) /
                                         b.pageSize);
                    page = &nextPage;
                }
            }
        }

        auto end = ftell(mStream.get());
#if SNAPSHOT_PROFILE > 1
        printf("ram: index size: %d\n", (int)(end - start));
#endif
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
    struct Page;

public:
    struct Block {
        uint32_t pageSize;
        RAMBlock* ramBlock;

        uint8_t* hostPtr() const {
            return (uint8_t*)qemu_host_from_ram_block(ramBlock);
        }
    };

    RamLoader(std::vector<Block>&& blocks, QEMUFile* f);

    void join() {
        char c = 1;
        HANDLE_EINTR(write(mExitFd.get(), &c, sizeof(1)));
        mPagefaultThread.wait();
    }

    ~RamLoader() = default;

    void onVmStateChange(bool running, RunState state) {
        if (running && state == RUN_STATE_RUNNING) {
            // done();
            // mReadingThread.wait();
        }
    }

private:
    enum class State : uint8_t { Clean, Reading, Read, Copying, Loaded, Error };

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

    struct Index {
        std::vector<Block> blocks;
        std::vector<Page> pages;
    };

    void zeroOutPage(const Block& block, uint32_t offset) {
        auto ptr = block.hostPtr() + (uint64_t)offset * block.pageSize;
        if (!isPageZeroed(ptr, block.pageSize)) {
            memset(ptr, 0, block.pageSize);
        }
    }

    void readIndex() {
#if SNAPSHOT_PROFILE > 1
        auto start = System::get()->getHighResTimeUs();
#endif
        mStreamFd = fileno(mStream.get());
        uint64_t indexPos = mStream.getBe64();
        fseek(mStream.get(), indexPos, SEEK_SET);

        int nonzeroPageCount = mStream.getBe32();
        mIndex.pages.reserve(nonzeroPageCount);
        for (Block& block : mIndex.blocks) {
            if (uint32_t zeroPagesCount = mStream.getBe32()) {
                uint32_t offset = mStream.getBe32();
                zeroOutPage(block, offset);
                for (uint32_t j = 1; j != zeroPagesCount; ++j) {
                    auto diff = mStream.getPackedNum();
                    offset += diff;
                    zeroOutPage(block, offset);
                }
            }

            if (uint32_t blockPagesCount = mStream.getBe32()) {
                const auto blockIndex = &block - mIndex.blocks.data();
                mIndex.pages.emplace_back();
                Page& page = mIndex.pages.back();
                page.blockIndex = blockIndex;
                page.state = uint8_t(State::Clean);
                uint32_t offset = page.offset = mStream.getBe32();
                uint64_t pos = page.filePos = mStream.getBe64();
                for (uint32_t j = 1; j != blockPagesCount; ++j) {
                    const auto offsetDiff = mStream.getPackedNum();
                    const auto posDiff = mStream.getPackedNum();
                    offset += offsetDiff;
                    pos += posDiff * block.pageSize;

                    mIndex.pages.emplace_back();
                    Page& page = mIndex.pages.back();
                    page.blockIndex = blockIndex;
                    page.state = uint8_t(State::Clean);
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

    uint32_t pageSize(const Page& page) const {
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
        if (!page.state.compare_exchange_strong(state,
                                                (uint8_t)State::Reading)) {
            // Spin until the reading thread finishes.
            while (state < uint8_t(State::Read)) {
                state = uint8_t(page.state.load());
            }
            return false;
        }

        auto size = pageSize(page);
        uint8_t* buf = bufGetter(size);
        auto read = (ssize_t)HANDLE_EINTR(
                pread(mStreamFd, buf, size, page.filePos));
        if (read != (ssize_t)size) {
            printf("WTF: %d %d %p %d\n", (int)read, (int)size, buf, errno);
            page.state.store(uint8_t(State::Error));
            return false;
        }

        page.data = buf;
        page.state.store(uint8_t(State::Read));
        return true;
    }

    template <class DataDeleter>
    void loadPage(Page* pagePtr, DataDeleter deleter) {
        Page& page = *pagePtr;
        auto state = uint8_t(State::Read);
        if (!page.state.compare_exchange_strong(state,
                                                uint8_t(State::Copying))) {
            assert(state == uint8_t(State::Loaded));
            return;
        }

#if SNAPSHOT_PROFILE > 2
        printf("loading page %p\n", this->pagePtr(page));
#endif
        uffdio_copy copyStruct = {(uint64_t)(uintptr_t)this->pagePtr(page),
                                  (uint64_t)(uintptr_t)page.data,
                                  pageSize(page)};
        if (ioctl(mUserfaultFd.get(), UFFDIO_COPY, &copyStruct)) {
            int e = errno;
            printf("%s: %s copy host: %p from: %p\n", __func__, strerror(e),
                   (void*)copyStruct.dst, (void*)copyStruct.src);
            deleter(page.data);
            page.data = nullptr;
            page.state.store(uint8_t(State::Error));
            return;
        }

        deleter(page.data);
        page.data = nullptr;

        uffdio_range rangeStruct{(uintptr_t)this->pagePtr(page),
                                 pageSize(page)};
        if (ioctl(mUserfaultFd.get(), UFFDIO_UNREGISTER, &rangeStruct)) {
            printf("%s: userfault unregister %s", __func__, strerror(errno));
            page.state.store(uint8_t(State::Error));
            return;
        }

        page.state.store(uint8_t(State::Loaded));
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

    void* readNextPagefaultAddr() const {
        uffd_msg msg;
        const int ret =
                HANDLE_EINTR(read(mUserfaultFd.get(), &msg, sizeof(msg)));
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
                return nullptr; /* Lost alignment, don't know what we'd read
                                   next */
            }
        }
        if (msg.event != UFFD_EVENT_PAGEFAULT) {
            printf("%s: Read unexpected event %ud from userfaultfd", __func__,
                   msg.event);
            return nullptr; /* It's not a page fault, shouldn't happen */
        }

        return (void*)(uintptr_t)msg.arg.pagefault.address;
    }

    void pagefaultWorker();

    bool registerMemoryRange(void* start, size_t length) {
        madvise(start, length, MADV_DONTNEED);
        uffdio_register regStruct = {{(uintptr_t)start, length},
                                     UFFDIO_REGISTER_MODE_MISSING};

        /* Now tell our userfault_fd that it's responsible for this area
         */
        if (ioctl(mUserfaultFd.get(), UFFDIO_REGISTER, &regStruct)) {
            printf("%s userfault register: %s", __func__, strerror(errno));
            return false;
        }
        return true;
    }

    std::string mUuid;
    StdioStream mStream;
    int mStreamFd;
    ScopedFd mUserfaultFd;
    ScopedFd mExitFd;

    OnExitFunctorThread mPagefaultThread;
    FunctorThread mReaderThread;

    MessageChannel<Page*, 32> mReadingQueue;
    MessageChannel<Page*, 32> mReadDataQueue;

    Index mIndex;

#if SNAPSHOT_PROFILE > 1
    System::WallDuration mStartTime;
#endif
};

static LazyInstance<std::vector<RamLoader::Block>> sRamLoaderBlocks = {};
static LazyInstance<std::unique_ptr<RamLoader>> sRamLoader = {};

RamLoader::RamLoader(std::vector<Block>&& blocks, QEMUFile* f)
    : mUuid(readRamFileUuid(f)),
      mStream(fopen(makeRamSaveFilename(mUuid).c_str(), "rb"),
              StdioStream::kOwner),
      mPagefaultThread([this]() { pagefaultWorker(); },
                       [] { sRamLoader->reset(); }),
      mReaderThread([this]() { readerWorker(); }) {
#if SNAPSHOT_PROFILE > 1
    mStartTime = System::get()->getHighResTimeUs();
#endif
    mIndex.blocks = std::move(blocks);
    readIndex();
    setupProtection();
}

void RamLoader::setupProtection() {
#if SNAPSHOT_PROFILE > 1
    auto start = System::get()->getHighResTimeUs();
#endif

    mUserfaultFd = ScopedFd(syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK));
    assert(mUserfaultFd.get() >= 0);

    uffdio_api apiStruct = {UFFD_API};
    if (ioctl(mUserfaultFd.get(), UFFDIO_API, &apiStruct)) {
        printf("UFFDIO_API failed: %s", strerror(errno));
        return;
    }

    uint64_t ioctlMask =
            (__u64)1 << _UFFDIO_REGISTER | (__u64)1 << _UFFDIO_UNREGISTER;
    if ((apiStruct.ioctls & ioctlMask) != ioctlMask) {
        printf("Missing userfault features: %" PRIx64,
               (uint64_t)(~apiStruct.ioctls & ioctlMask));
        return;
    }

    mExitFd = ScopedFd(eventfd(0, EFD_CLOEXEC));

    uint8_t* startPtr = nullptr;
    uint32_t curSize = 0;
    for (const Page& page : mIndex.pages) {
        auto ptr = pagePtr(page);
        auto size = pageSize(page);
        if (ptr == startPtr + curSize) {
            curSize += size;
        } else {
            if (startPtr) {
                if (!registerMemoryRange(startPtr, curSize)) {
                    return;
                }
            }
            startPtr = ptr;
            curSize = size;
        }
    }
    if (startPtr) {
        if (!registerMemoryRange(startPtr, curSize)) {
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

void RamLoader::pagefaultWorker() {
    auto it = mIndex.pages.begin();
    for (;;) {
        pollfd pfd[2] = {{mUserfaultFd.get(), POLLIN}, {mExitFd.get(), POLLIN}};
        timespec timeout = {0, 1000};
        if (it == mIndex.pages.end() ||
            mReadingQueue.size() == mReadingQueue.capacity()) {
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
            while (auto ptr = readNextPagefaultAddr()) {
                page = &this->page(ptr);
                assert(page->state < (uint8_t)State::Loaded);
                uint8_t buf[4096];
                assert(sizeof(buf) >= pageSize(*page));
                readData(page, [&buf](int) { return buf; });
                loadPage(page, [&buf](uint8_t* ptr) {
                    if (ptr != &buf[0]) {
                        delete[] ptr;
                    }
                });
            }
        } else if (mReadDataQueue.tryReceive(&page)) {
            if (page) {
                loadPage(page, [](uint8_t* ptr) { delete[] ptr; });
            } else {
                mReadDataQueue.stop();
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

static QEMUFileHooks sSaveHooks = {
        .before_ram_iterate =
                [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
                    qemu_put_be64(f, RAM_SAVE_FLAG_HOOK);
                    if (flags == RAM_CONTROL_SETUP) {
                        sRamSaver->reset(new RamSaver(f));
                    }
                    return 0;
                },
        .after_ram_iterate =
                [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
                    if (flags == RAM_CONTROL_FINISH) {
                        sRamSaver->reset();
                    }
                    return 0;
                },
        .hook_ram_load = nullptr,
        .save_page = nullptr,
        .save_page_ex = [](QEMUFile* f,
                           void* opaque,
                           ram_addr_t block_offset,
                           const char* block_id,
                           const void* block_host_ptr,
                           ram_addr_t offset,
                           size_t size,
                           uint64_t* bytes_sent) -> size_t {
            sRamSaver.get()->savePage(block_offset, block_id, block_host_ptr,
                                      offset, size);
            *bytes_sent = 1;
            return RAM_SAVE_CONTROL_DELAYED;
        },
};

static QEMUFileHooks sLoadHooks = {
        .before_ram_iterate = nullptr,
        .after_ram_iterate = nullptr,
        .hook_ram_load =
                [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
                    switch (flags) {
                        case RAM_CONTROL_BLOCK_REG: {
                            if (sRamLoaderBlocks->empty()) {
                                // First call to the load hook - finalize the
                                // previous session.
                                sRamLoader->reset();
                            }
                            const auto name = static_cast<const char*>(data);
                            RAMBlock* block = qemu_ram_block_by_name(name);
                            sRamLoaderBlocks->push_back(
                                    {(uint32_t)qemu_ram_pagesize(block),
                                     block});
                            break;
                        }
                        case RAM_CONTROL_HOOK:
                            if (!sRamLoader->get()) {
                                sRamLoader->reset(new RamLoader(
                                        std::move(sRamLoaderBlocks.get()), f));
                            }
                            break;
                    }
                    return 0;
                }};

void qemu_snapshot_hooks_setup() {
    migrate_set_file_hooks(&sSaveHooks, &sLoadHooks);

    qemu_add_vm_change_state_handler(
            [](void* opaque, int running, RunState state) {
                if (auto rs = sRamSaver->get()) {
                    rs->onVmStateChange(running != 0, state);
                }
                if (auto rl = sRamLoader->get()) {
                    rl->onVmStateChange(running != 0, state);
                }
            },
            nullptr);
}
