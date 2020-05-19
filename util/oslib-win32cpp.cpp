// this provide some functions written in cpp for windows
// such as wait for more than 64 objects
// work in progress
// main idea:
// using std::async to use the thread pool on windows
// so to avoid creation of threads in g_poll (windows version)

extern "C" {
#include <windows.h>

#include "qemu/osdep.h"
}

#include <stdio.h>

#include <future>
#include <vector>


#define YUV_CONVERTER_DEBUG 1

#if YUV_CONVERTER_DEBUG
#define DDD(fmt, ...)                                                       \
  fprintf(stderr, "windows wait: %s:%d thread id %llu " fmt "\n", __func__, \
          __LINE__, std::this_thread::get_id(), ##__VA_ARGS__);
#else
#define DDD(fmt, ...)
#endif


namespace {

constexpr int num_fds_per_item = 60;

struct WorkItem {
  std::vector<GPollFD> gfds;
  HANDLE stopSignal;
  HANDLE doneSignal;
  int timeout;
};

std::vector<WorkItem> divideWorkItems(GPollFD* fds, int nfds) {
  int nwork = nfds / num_fds_per_item;
  int remain = nfds % num_fds_per_item;
  if (remain > 0) ++nwork;

  std::vector<WorkItem> items(nwork);
  for (int i = 0; i < nfds; ++i) {
    int index = i / num_fds_per_item;
    auto& item = items[index];
    item.gfds.push_back(fds[i]);
  }
  return items;
}

int combineWork(std::vector<WorkItem>& items, GPollFD* gfds, int nfds) {
  int numReady = 0;
  for (int i = 0; i < nfds; ++i) {
    int index = i / num_fds_per_item;
    auto& item = items[index];
    gfds[i] = item.gfds[i % num_fds_per_item];
    if (gfds[i].revents != 0) {
      DDD("found fd %llu with event %d", gfds[i].fd, gfds[i].revents);
      ++numReady;
    }
  }

  return numReady;
}

int one_wait_job(WorkItem* pitem) {
  auto& item = *pitem;
  constexpr int array_size = 64;
  HANDLE handles[array_size];

  assert(item.gfds.size() < array_size);
  int nfds = 0;
  handles[nfds++] = item.stopSignal;
  for (int i = 0; i < item.gfds.size(); ++i) {
    auto& gfd = item.gfds[i];
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
      auto& gfd = item.gfds[indexOriginal - 1];
      gfd.revents = gfd.events;
      DDD("found revents %d handle %llu", gfd.revents, gfd.fd);
      ++numReady;
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
  DDD("this thread returns with %d", numReady);
  return numReady;
}

int wait_async(std::vector<WorkItem>& items, int timeout) {
  auto stopSignal = CreateEventW(NULL, TRUE, FALSE, NULL);
  std::vector<std::future<int>> results(items.size());
  std::vector<HANDLE> thread_done_signals(items.size());
  for (int i = 0; i < items.size(); ++i) {
    auto handle = CreateEventW(NULL, TRUE, FALSE, NULL);
    thread_done_signals[i] = handle;
    items[i].stopSignal = stopSignal;
    items[i].doneSignal = handle;
    items[i].timeout = timeout;
  DDD("started one async work");
    results[i] = std::async(std::launch::async, one_wait_job, &(items[i]));
  }

  DDD("wait for any thread to complete");
  // wait for any threads to signal done
  int ret = WaitForMultipleObjects(thread_done_signals.size(),
                                   thread_done_signals.data(), FALSE,
                                   items[0].timeout);

  // if there is any thread signaled, stop all the others
  SetEvent(stopSignal);

  DDD("wait for all threads to complete");
  // wait for all to signal
  ret = WaitForMultipleObjects(thread_done_signals.size(),
                               thread_done_signals.data(), TRUE, INFINITE);

  DDD("wait drain all the futures");
  // drain the futures
  for (auto& r : results) {
    r.get();
  }

  return 0;
}

int wait_driver(GPollFD* fds, int nfds, int timeout) {
  DDD("start driver... id ");
  auto items = divideWorkItems(fds, nfds);

    DDD("start wait_async");
  wait_async(items, timeout);

    DDD("start combine");
  int numReady = combineWork(items, fds, nfds);

    DDD("done %d", numReady);
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

