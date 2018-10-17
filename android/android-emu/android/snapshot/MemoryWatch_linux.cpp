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

#include "android/snapshot/MemoryWatch.h"

#include "android/base/ArraySize.h"
#include "android/base/Debug.h"
#include "android/base/EintrWrapper.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/threads/FunctorThread.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/utils/debug.h"

#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <cassert>
#include <utility>

// Looks like the current emulator toolchain doesn't have this define.
#ifndef __NR_userfaultfd
#ifdef __i368__
#define __NR_userfaultfd 374
#elif defined(__x86_64__)
#define __NR_userfaultfd 323
#else
#error This target architecture is not supported
#endif
#endif

namespace fc = android::featurecontrol;
using fc::Feature;

namespace android {
namespace snapshot {

static bool checkUserfaultFdCaps(int ufd) {
    if (ufd < 0) {
        return false;
    }

    uffdio_api apiStruct;
    memset(&apiStruct, 0x0, sizeof(uffdio_api));
    apiStruct.api = UFFD_API;

    if (ioctl(ufd, UFFDIO_API, &apiStruct)) {
        dwarning("UFFDIO_API failed: %s", strerror(errno));
        return false;
    }

    uint64_t ioctlMask = 1ull << _UFFDIO_REGISTER | 1ull << _UFFDIO_UNREGISTER;

    if (fc::isEnabled(Feature::QuickbootFileBacked)) {
        // It's possible for UFFD_FEATURE_MISSING_SHMEM to
        // be returned in features but registering ranges
        // still fail. To be safe, return false unconditionally.
        return false;
    }

    if ((apiStruct.ioctls & ioctlMask) != ioctlMask) {
        dwarning(
                "Missing userfault features: %llu",
                static_cast<unsigned long long>(~apiStruct.ioctls & ioctlMask));
        return false;
    }

    return true;
}

class MemoryAccessWatch::Impl {
public:
    Impl(MemoryAccessWatch::AccessCallback&& accessCallback,
         MemoryAccessWatch::IdleCallback&& idleCallback)
        : mAccessCallback(std::move(accessCallback)),
          mIdleCallback(std::move(idleCallback)),
          mPagefaultThread([this]() { pagefaultWorker(); }) {
        mUserfaultFd = base::ScopedFd(
                int(syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK)));
        if (!checkUserfaultFdCaps(mUserfaultFd.get())) {
            mUserfaultFd.close();
        }
        mExitFd = base::ScopedFd(eventfd(0, EFD_CLOEXEC));
        assert(mExitFd.get() >= 0);
    }

    void join() { mPagefaultThread.wait(); }

    ~Impl() { join(); }

    void* readNextPagefaultAddr() const {
        uffd_msg msg;
        const auto ret =
                HANDLE_EINTR(read(mUserfaultFd.get(), &msg, sizeof(msg)));
        if (ret != sizeof(msg)) {
            if (errno == EAGAIN) {
                /* if a wake up happens on the other thread just after
                 * the poll, there is nothing to read. */
                return nullptr;
            }
            if (ret < 0) {
                derror("%s: Failed to read full userfault message: %s",
                       __func__, strerror(errno));
                return nullptr;
            } else {
                derror("%s: Read %d bytes from userfaultfd expected %zd",
                       __func__, ret, sizeof(msg));
                return nullptr; /* Lost alignment, don't know what we'd read
                                   next */
            }
        }
        if (msg.event != UFFD_EVENT_PAGEFAULT) {
            derror("%s: Read unexpected event %ud from userfaultfd", __func__,
                   msg.event);
            return nullptr; /* It's not a page fault, shouldn't happen */
        }
        return reinterpret_cast<void*>(uintptr_t(msg.arg.pagefault.address));
    }

    void pagefaultWorker() {
        assert(mUserfaultFd.valid());
        int timeoutNs = 0;
        // Use the 'done' flag to run one more iteration on the userspacefd
        // descriptor after unregistering all ranges: this way we can be sure
        // there's no pending requests left.
        bool done = false;
        for (;;) {
            pollfd pfd[] = {{mExitFd.get(), POLLIN},
                            {mUserfaultFd.get(), POLLIN}};
            timespec timeout = {0, timeoutNs};
            if (ppoll(pfd, ARRAY_SIZE(pfd), &timeout, nullptr) == -1) {
                derror("%s: userfault ppoll: %s", __func__, strerror(errno));
                break;
            }
            if (pfd[1].revents) {
                while (auto ptr = readNextPagefaultAddr()) {
                    mAccessCallback(ptr);
                }
                timeoutNs = 0;
            }

            if (done) {
                break;
            }

            if (pfd[0].revents) {
                unregisterAll();
                done = true;
                continue;
            }

            if (!pfd[1].revents) {
                switch (mIdleCallback()) {
                    case IdleCallbackResult::RunAgain:
                        timeoutNs = 0;
                        break;
                    case IdleCallbackResult::Wait:
                        timeoutNs = 500 * 1000;
                        break;
                    case IdleCallbackResult::AllDone:
                        unregisterAll();
                        done = true;
                        continue;
                }
            }
        }
    }

    void unregisterAll() {
        for (auto&& range : mRanges) {
            uffdio_range rangeStruct{(uintptr_t)range.first, range.second};
            if (ioctl(mUserfaultFd.get(), UFFDIO_UNREGISTER, &rangeStruct)) {
                derror("%s: userfault unregister %p - %s", __func__,
                       range.first, strerror(errno));
            }
        }
        mRanges.clear();
    }

    void stop() { HANDLE_EINTR(eventfd_write(mExitFd.get(), 1)); }

    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;

    base::ScopedFd mUserfaultFd;
    base::ScopedFd mExitFd;

    std::vector<std::pair<void*, uint64_t>> mRanges;

    base::FunctorThread mPagefaultThread;
};

bool MemoryAccessWatch::isSupported() {
    if (!android::featurecontrol::isEnabled(
            android::featurecontrol::OnDemandSnapshotLoad)) {
        return false;
    }

    if (android::featurecontrol::isEnabled(
            android::featurecontrol::QuickbootFileBacked)) {
        return false;
    }

    base::ScopedFd ufd(int(syscall(__NR_userfaultfd, O_CLOEXEC)));
    return checkUserfaultFdCaps(ufd.get());
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&& accessCallback,
                                     IdleCallback&& idleCallback)
    : mImpl(new Impl(std::move(accessCallback), std::move(idleCallback))) {}

MemoryAccessWatch::~MemoryAccessWatch() {
    mImpl->stop();
}

bool MemoryAccessWatch::valid() const {
    return mImpl->mUserfaultFd.valid();
}

bool MemoryAccessWatch::registerMemoryRange(void* start, size_t length) {
    uffdio_register regStruct = {{(uintptr_t)start, length},
                                 UFFDIO_REGISTER_MODE_MISSING};
    if (ioctl(mImpl->mUserfaultFd.get(), UFFDIO_REGISTER, &regStruct)) {
        derror("%s userfault register(%p, %d): %s", __func__, start,
               int(length), strerror(errno));
        mImpl->mUserfaultFd.close();
        return false;
    }
    madvise(start, length, MADV_DONTNEED);

    mImpl->mRanges.emplace_back(start, length);
    return true;
}

void MemoryAccessWatch::doneRegistering() {
    if (valid()) {
        mImpl->mPagefaultThread.start();
    }
}

bool MemoryAccessWatch::fillPage(void* ptr,
                                 size_t length,
                                 const void* data,
                                 bool isQuickboot) {
    if (data) {
        uffdio_copy copyStruct = {uintptr_t(ptr), uintptr_t(data), length};
        if (ioctl(mImpl->mUserfaultFd.get(), UFFDIO_COPY, &copyStruct)) {
            derror("%s: %s copy host: %p from: %p\n", __func__, strerror(errno),
                   reinterpret_cast<void*>(copyStruct.dst),
                   reinterpret_cast<void*>(copyStruct.src));
            return false;
        }
    } else {
        uffdio_zeropage zeroStruct = {uintptr_t(ptr), length};
        if (ioctl(mImpl->mUserfaultFd.get(), UFFDIO_ZEROPAGE, &zeroStruct)) {
            derror("%s: %s zero host: %p\n", __func__, strerror(errno),
                   reinterpret_cast<void*>(zeroStruct.range.start));
            return false;
        }
    }

    // TODO: figure out why this kills emulator on some Linux machines.
    //
    // uffdio_range rangeStruct{(uintptr_t)ptr, length};
    // if (ioctl(mImpl->mUserfaultFd.get(), UFFDIO_UNREGISTER, &rangeStruct)) {
    //     derror("%s: userfault unregister %p - %s", __func__, ptr,
    //            strerror(errno));
    // }
    //
    return true;
}

void MemoryAccessWatch::initBulkFill(void* startPtr, size_t length) {
    // TODO: no-op for now
}

bool MemoryAccessWatch::fillPageBulk(void* startPtr, size_t length, const void* data,
              bool isQuickboot) {
    return fillPage(startPtr, length, data, isQuickboot);
}

void MemoryAccessWatch::join() {
    if (mImpl) {
        mImpl->join();
    }
}

}  // namespace snapshot
}  // namespace android
