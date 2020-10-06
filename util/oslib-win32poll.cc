// Copyright 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// this implements multi-threaded WaitForMultipleObjects on windows
// for more than 64 objects
//
// main idea:
// use thread pools to wait on condition variable that is send out
// whenever there are work to do from calling thread.
// 0. when the number of fds is <= 64, use the old implementation
//    in oslib-win32.c where a single call of WaitForMultipleObjects is used,
//    that is the fastest; multi-threading does not help, as the fired
//    events are very sparse. synchronization cost using mt is over-kill
// 1. use as few threads as possible, and once started keep them persistent
//    and wait for new job. std::async is better avoided, as its performance
//    is not predictable. based on testing, it is important to use thread pool
//    as the alternative is taking 0.5ms on windows, too much for poll,
//    translates to 20% to 30% boot time increase
// 2. once the minimum number of threads are decided, the work load should be
//    balanced among them.
// 3. need to avoid vector create/destroy as well; now the performance is almost
//    on par with singla wait for fewer than 64 fds (about 5% slower)
// 4. G_WIN32_MSG_HANDLE is not handled, as it does not happen on emulator
//

extern "C" {
#include <windows.h>

#include "qemu/osdep.h"
}

#include <stdio.h>

#include <condition_variable>
#include <thread>
#include <vector>

#define WIN32_POLL_DEBUG 0

#if WIN32_POLL_DEBUG
#define DD(fmt, ...)                                                           \
    fprintf(stderr, "win32poll: %s:%d thread id %llu " fmt "\n", __func__,     \
            __LINE__, std::this_thread::get_id(), ##__VA_ARGS__);
#else
#define DD(fmt, ...)
#endif

namespace {

constexpr int kNumFdsPerItem = 60;
constexpr int kMaxNumFdsPerItem = MAXIMUM_WAIT_OBJECTS;

struct WorkItem {
    GPollFD *pfds{nullptr};
    HANDLE handles[kMaxNumFdsPerItem];
    HANDLE stopSignal;
    HANDLE doneSignal;
    int nfds{0};
    int fired{0};
    int timeout;
};

struct PollWorkerState {
    std::vector<std::unique_ptr<std::condition_variable>> s_cv;
    std::vector<std::unique_ptr<std::mutex>> s_cv_m;
    bool s_should_quit{false};
    HANDLE s_stopSignal;

    std::vector<HANDLE> s_thread_done_signals;

    std::vector<std::thread> s_threads;
    std::vector<WorkItem *> s_works;
    std::vector<WorkItem> s_real_works;
};

static PollWorkerState sPollWorkerState;

int one_wait_job(WorkItem *pitem) {
    auto &item = *pitem;
    constexpr int array_size = kMaxNumFdsPerItem;
    HANDLE *handles = item.handles;

    assert(item.nfds < array_size);
    int nfds = 0;
    for (int i = 0; i < item.nfds; ++i) {
        auto &gfd = item.pfds[i];
        handles[nfds++] = reinterpret_cast<HANDLE>(gfd.fd);
        gfd.revents = 0;
    }

    // put the stop signal at the end, so to avoid
    // 1 wait in terms of timeout
    handles[nfds++] = item.stopSignal;

    HANDLE *phandles = handles;
    int ret = WaitForMultipleObjects(nfds, phandles, FALSE, item.timeout);
    if (ret == WAIT_FAILED) {
        item.fired = 0;
        SetEvent(item.doneSignal);
        return -1;
    }
    if (ret == WAIT_TIMEOUT) {
        item.fired = 0;
        SetEvent(item.doneSignal);
        return 0;
    }

    int numReady = 0;
    while (ret >= WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + nfds) {
        const int offset = phandles - handles;
        const int indexFired = ret - WAIT_OBJECT_0;
        const int indexOriginal = indexFired + offset;
        if (handles[indexOriginal] != item.stopSignal) {
            auto &gfd = item.pfds[indexOriginal];
            gfd.revents = gfd.events;
            DD("found revents %d handle %llu", gfd.revents, gfd.fd);
            if (gfd.revents != 0) {
                ++numReady;
            }
        }

        // prepare for next check: skip the fired one (1) and the ones before it
        // (indexFired)
        const int num_handles_to_ignore = (indexFired + 1);
        nfds -= num_handles_to_ignore;
        phandles += num_handles_to_ignore;
        constexpr int timeout = 0; // no wait

        if (nfds == 0) {
            break;
        }

        ret = WaitForMultipleObjects(nfds, phandles, FALSE, timeout);
    }

    SetEvent(item.doneSignal);
    DD("this thread returns with %d", numReady);
    item.fired = numReady;

    return numReady;
}

void single_thread_work(int idx) {
    while (!sPollWorkerState.s_should_quit) {
        DD("started work thread %d", idx);
        WorkItem *pitem = nullptr;
        {
            std::unique_lock<std::mutex> lk(*(sPollWorkerState.s_cv_m[idx]));
            sPollWorkerState.s_cv[idx]->wait(lk, [idx] {
                return sPollWorkerState.s_works[idx] != nullptr ||
                       sPollWorkerState.s_should_quit;
            });
            pitem = sPollWorkerState.s_works[idx];
            sPollWorkerState.s_works[idx] = nullptr;
        }
        if (sPollWorkerState.s_should_quit) {
            return;
        }

        // get work
        DD("really work now work thread %d", idx);
        one_wait_job(pitem);
    }
}

int wait_async(GPollFD *fds, int nfds, std::vector<WorkItem> &items,
               int timeout, bool need_mt) {
    if (sPollWorkerState.s_works.empty()) {
        // one time only
        // note, we dont want to change this vector when more threads are added,
        // as existing threads are waiting on it to become non-nullptr
        // so just allocate the maximum numbers and done with it
        sPollWorkerState.s_works.resize(kMaxNumFdsPerItem, nullptr);
        sPollWorkerState.s_cv_m.resize(kMaxNumFdsPerItem);
        sPollWorkerState.s_cv.resize(kMaxNumFdsPerItem);

        // the one and only one stop signal
        sPollWorkerState.s_stopSignal = CreateEventW(NULL, TRUE, FALSE, NULL);
    }

    int nwork = 1;
    int x_kNumFdsPerItem = kNumFdsPerItem;

    if (1) {
        static_assert(kNumFdsPerItem < kMaxNumFdsPerItem);
        static_assert(kMaxNumFdsPerItem <= MAXIMUM_WAIT_OBJECTS);
        const int remain = nfds % kNumFdsPerItem;
        nwork = nfds / kNumFdsPerItem + ((remain > 0) ? 1 : 0);

        x_kNumFdsPerItem = kNumFdsPerItem;

        if (nwork > 1) {
            // rebalance the work load for each thread;
            x_kNumFdsPerItem = nfds / nwork;
            if (x_kNumFdsPerItem * nwork != nfds) {
                x_kNumFdsPerItem +=
                    1; // add one so the last one does not overflow
            }
            assert(x_kNumFdsPerItem < kMaxNumFdsPerItem);
            DD("rebalancing: work per item is %d fds", x_kNumFdsPerItem);
        }

        if (items.size() < nwork) {
            items.resize(nwork);
        }
        DD("size of real_works %d", items.size());
    }

    // need to reset sPollWorkerState.s_stopSignal each time
    ResetEvent(sPollWorkerState.s_stopSignal);

    for (int i = sPollWorkerState.s_thread_done_signals.size(); i < nwork;
         ++i) {
        auto handle = CreateEventW(NULL, TRUE, FALSE, NULL);
        sPollWorkerState.s_thread_done_signals.push_back(handle);
        sPollWorkerState.s_cv_m[i].reset(new std::mutex);
        sPollWorkerState.s_cv[i].reset(new std::condition_variable);
        sPollWorkerState.s_threads.emplace_back(
            std::move(std::thread(single_thread_work, i)));
    }

    for (int i = 0; i < nwork; ++i) {
        DD("prepare for %d th item", i);
        auto handle = sPollWorkerState.s_thread_done_signals[i];
        ResetEvent(handle);
        items[i].stopSignal = sPollWorkerState.s_stopSignal;
        items[i].doneSignal = handle;
        items[i].timeout = timeout;
        GPollFD *pfds = fds + i * x_kNumFdsPerItem;
        const int num_fds =
            (i == nwork - 1) ? (nfds - i * x_kNumFdsPerItem) : x_kNumFdsPerItem;
        assert(num_fds + 1 <= kMaxNumFdsPerItem);
        items[i].pfds = pfds;
        items[i].nfds = num_fds;
        if (need_mt) {
            std::lock_guard<std::mutex> lk(*(sPollWorkerState.s_cv_m[i]));
            sPollWorkerState.s_works[i] = &(items[i]);
            sPollWorkerState.s_cv[i]->notify_all();
        } else {
            one_wait_job(&(items[i]));
        }
    }

    if (need_mt) {
        DD("wait for any thread to complete");
        // wait for any threads to signal done
        int ret = WaitForMultipleObjects(
            nwork, sPollWorkerState.s_thread_done_signals.data(), FALSE,
            items[0].timeout);

        // if there is any thread signaled, stop all the others
        SetEvent(sPollWorkerState.s_stopSignal);

        DD("wait for all threads to complete");
        // wait for all to signal
        ret = WaitForMultipleObjects(
            nwork, sPollWorkerState.s_thread_done_signals.data(), TRUE,
            INFINITE);
    }

    int numReady = 0;
    for (int i = 0; i < nwork; ++i) {
        numReady += items[i].fired;
    }
    return numReady;
}

int wait_driver(GPollFD *fds, int nfds, int timeout) {
    DD("start driver... nfds %d", nfds);

    DD("start wait_async");
    bool need_mt = timeout > 0;
    if (nfds <= kNumFdsPerItem) {
        need_mt = false;
    }
    int numReady =
        wait_async(fds, nfds, sPollWorkerState.s_real_works, timeout, need_mt);

    DD("done %d", numReady);
    return numReady;
}

} // anonymous namespace

extern "C" {
// timeout: -1: wait infinite
// 0: no wait, just check fd status
// >=1: wait 1ms or more
int win32_wait_for_objects(GPollFD *fds, int nfds, int timeout) {
    if (timeout == -1) {
        timeout = INFINITE;
    }
    int ret = wait_driver(fds, nfds, timeout);
    return ret;
}
}
