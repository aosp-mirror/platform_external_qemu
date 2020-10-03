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
// 1. important to use thread pool as the alternative is taking 0.5ms
// on windows, too much for poll, translates to 20% to 30% boot time
// increase
// 2. need to avoid vector create/destroy as well
// now the performance is almost on par with singla wait for fewer than
// 3. G_WIN32_MSG_HANDLE is not handled, as it does not happen on emulator
//

extern "C" {
#include <windows.h>

#include "qemu/osdep.h"
}

#include <stdio.h>

#include <condition_variable>
#include <future>
#include <thread>
#include <vector>

#define WIN32_POLL_DEBUG 0

#if WIN32_POLL_DEBUG
#define DD(fmt, ...)                                                     \
  fprintf(stderr, "win32poll: %s:%d thread id %llu " fmt "\n", __func__, \
          __LINE__, std::this_thread::get_id(), ##__VA_ARGS__);
#else
#define DD(fmt, ...)
#endif

namespace {

std::condition_variable s_cv;
std::mutex s_cv_m;
bool s_should_quit = false;
bool s_has_work = false;

constexpr int num_fds_per_item = 60;
constexpr int max_num_fds_per_item = MAXIMUM_WAIT_OBJECTS;

struct WorkItem {
  GPollFD* pfds{nullptr};
  HANDLE handles[max_num_fds_per_item];
  HANDLE stopSignal;
  HANDLE doneSignal;
  int nfds{0};
  int fired{0};
  int timeout;
};

std::vector<std::thread> s_threads;
std::vector<WorkItem*> s_works;
std::vector<WorkItem> s_real_works;

void divideWorkItems(GPollFD* fds, int nfds) {
  static_assert(num_fds_per_item < max_num_fds_per_item);
  static_assert(max_num_fds_per_item <= MAXIMUM_WAIT_OBJECTS);

  const int remain = nfds % num_fds_per_item;
  const int nwork = nfds / num_fds_per_item + ((remain > 0) ? 1 : 0);

  int x_num_fds_per_item = num_fds_per_item;

  if (nwork > 1) {
    // rebalance the work load for each thread;
    x_num_fds_per_item = nfds / nwork;
    if (x_num_fds_per_item * nwork != nfds) {
      x_num_fds_per_item += 1;  // add one so the last one does not overflow
    }
    assert(x_num_fds_per_item < max_num_fds_per_item);
    DD("rebalancing: work per item is %d fds", x_num_fds_per_item);
  }

  if (s_real_works.size() < nwork) {
    s_real_works.resize(nwork);
    s_works.resize(nwork);
  }
  DD("size of s_real_works %d", s_real_works.size());

  for (int i = 0; i < nwork; ++i) {
    auto& item = s_real_works[i];
    GPollFD* pfds = fds + i * x_num_fds_per_item;
    const int num_fds =
        (i == nwork - 1) ? (nfds - i * x_num_fds_per_item) : x_num_fds_per_item;
    assert(num_fds + 1 <= max_num_fds_per_item);
    item.pfds = pfds;
    item.nfds = num_fds;
    DD("setting up %d th work fds %d", i, num_fds);
  }
}

int combineWork(std::vector<WorkItem>& items, GPollFD* gfds, int nfds) {
  int numReady = 0;
  for (auto& item : items) {
    numReady += item.fired;
  }
  return numReady;
}

int one_wait_job(WorkItem* pitem) {
  auto& item = *pitem;
  constexpr int array_size = max_num_fds_per_item;
  HANDLE* handles = item.handles;

  assert(item.nfds < array_size);
  int nfds = 0;
  handles[nfds++] = item.stopSignal;
  for (int i = 0; i < item.nfds; ++i) {
    auto& gfd = item.pfds[i];
    handles[nfds++] = reinterpret_cast<HANDLE>(gfd.fd);
    gfd.revents = 0;
  }

  HANDLE* phandles = handles;
  int ret = WaitForMultipleObjects(nfds, phandles, FALSE, item.timeout);
  if (ret == WAIT_FAILED) {
    SetEvent(item.doneSignal);
    return -1;
  }
  if (ret == WAIT_TIMEOUT) {
    SetEvent(item.doneSignal);
    return 0;
  }

  int numReady = 0;
  while (ret >= WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + nfds) {
    const int offset = phandles - handles;
    const int indexFired = ret - WAIT_OBJECT_0;
    const int indexOriginal = indexFired + offset;
    if (handles[indexOriginal] != item.stopSignal) {
      auto& gfd = item.pfds[indexOriginal - 1];
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
    constexpr int timeout = 0;  // no wait

    ret = WaitForMultipleObjects(nfds, phandles, FALSE, timeout);
  }

  SetEvent(item.doneSignal);
  DD("this thread returns with %d", numReady);
  item.fired = numReady;

  return numReady;
}

void single_thread_work(int idx) {
  while (!s_should_quit) {
    DD("started work thread %d", idx);
    WorkItem* pitem = nullptr;
    {
      std::unique_lock<std::mutex> lk(s_cv_m);
      s_cv.wait(lk, [idx] { return s_works[idx] != nullptr || s_should_quit; });
      pitem = s_works[idx];
      s_works[idx] = nullptr;
    }
    if (s_should_quit) {
      return;
    }

    // get work
    DD("really work now work thread %d", idx);
    one_wait_job(pitem);
  }
}

void start_work_thread(std::vector<WorkItem>& items) {
  const int num_threads = items.size();
  if (s_threads.size() < num_threads) {
    DD("increase number of threads to %d", num_threads);
    for (int i = s_threads.size(); i < num_threads; ++i) {
      s_threads.emplace_back(std::move(std::thread(single_thread_work, i)));
    }
  }

  DD("prepare work for all threads %d", num_threads);
  {
    std::lock_guard<std::mutex> lk(s_cv_m);
    for (int i = 0; i < items.size(); ++i) {
      s_works[i] = &(items[i]);
    }
    s_cv.notify_all();
  }
  DD("done notifying prepare work for all threads %d", num_threads);
}

int wait_async(std::vector<WorkItem>& items, int timeout, bool need_mt) {
  DD("prepare stopsignal num items %d", items.size());
  auto stopSignal = CreateEventW(NULL, TRUE, FALSE, NULL);
  std::vector<HANDLE> thread_done_signals(items.size());
  for (int i = 0; i < items.size(); ++i) {
    DD("prepare for %d th item", i);
    auto handle = CreateEventW(NULL, TRUE, FALSE, NULL);
    thread_done_signals[i] = handle;
    items[i].stopSignal = stopSignal;
    items[i].doneSignal = handle;
    items[i].timeout = timeout;
    if (need_mt) {
      DD("started one async work");
    } else {
      one_wait_job(&(items[i]));
    }
  }

  DD("step2");
  if (need_mt) {
    DD("trigger mt wait ");
    start_work_thread(items);
  }

  DD("step3");
  if (need_mt) {
    DD("wait for any thread to complete");
    // wait for any threads to signal done
    DD("step4");
    int ret = WaitForMultipleObjects(thread_done_signals.size(),
                                     thread_done_signals.data(), FALSE,
                                     items[0].timeout);

    // if there is any thread signaled, stop all the others
    SetEvent(stopSignal);

    DD("step5");
    DD("wait for all threads to complete");
    // wait for all to signal
    ret = WaitForMultipleObjects(thread_done_signals.size(),
                                 thread_done_signals.data(), TRUE, INFINITE);
  }
  DD("wait drain all the futures");

  return 0;
}

int wait_driver(GPollFD* fds, int nfds, int timeout) {
  DD("start driver... nfds %d", nfds);
  divideWorkItems(fds, nfds);

  DD("start wait_async");
  bool need_mt = timeout > 0;
  if (nfds <= num_fds_per_item) {
    need_mt = false;
  }
  wait_async(s_real_works, timeout, need_mt);

  DD("start combine");
  int numReady = combineWork(s_real_works, fds, nfds);

  DD("done %d", numReady);
  return numReady;
}

}  // anonymous namespace

extern "C" {
// timeout: -1: wait infinite
// 0: no wait, just check fd status
// >=1: wait 1ms or more
int win32_wait_for_objects(GPollFD* fds, int nfds, int timeout) {
  if (timeout == -1) {
    timeout = INFINITE;
  }
  int ret = wait_driver(fds, nfds, timeout);
  return ret;
}
}
