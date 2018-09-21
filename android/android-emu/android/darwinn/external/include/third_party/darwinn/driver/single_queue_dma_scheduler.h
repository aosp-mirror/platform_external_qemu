#ifndef THIRD_PARTY_DARWINN_DRIVER_SINGLE_QUEUE_DMA_SCHEDULER_H_
#define THIRD_PARTY_DARWINN_DRIVER_SINGLE_QUEUE_DMA_SCHEDULER_H_

#include <condition_variable>  // NOLINT
#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <queue>
#include <vector>

#include "third_party/darwinn/driver/dma_info.h"
#include "third_party/darwinn/driver/dma_scheduler.h"
#include "third_party/darwinn/driver/request.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Manages the processing order of DMAs with single queue. All DMAs are
// serialized. Thread-safe.
class SingleQueueDmaScheduler : public DmaScheduler {
 public:
  SingleQueueDmaScheduler() = default;
  ~SingleQueueDmaScheduler() override = default;

  // Implements DmaScheduler interfaces.
  util::Status Open() override LOCKS_EXCLUDED(mutex_);
  util::Status Close() override LOCKS_EXCLUDED(mutex_);
  util::Status Submit(std::shared_ptr<Request> request) override
      LOCKS_EXCLUDED(mutex_);
  DmaDescriptorType PeekNextDma() const override LOCKS_EXCLUDED(mutex_);
  DmaInfo* GetNextDma() override LOCKS_EXCLUDED(mutex_);
  util::Status NotifyDmaCompletion(DmaInfo* dma_info) override
      LOCKS_EXCLUDED(mutex_);
  util::Status NotifyRequestCompletion() override LOCKS_EXCLUDED(mutex_);
  util::Status CancelPendingRequests() override LOCKS_EXCLUDED(mutex_);
  util::Status WaitActiveRequests() override LOCKS_EXCLUDED(mutex_);
  bool IsEmpty() const override LOCKS_EXCLUDED(mutex_) {
    StdMutexLock lock(&mutex_);
    return IsEmptyLocked();
  }

 private:
  // A data structure for managing Request and associated DMAs.
  struct Task {
    Task(std::shared_ptr<Request> request, std::list<DmaInfo>&& dmas)
        : request(std::move(request)), dmas(std::move(dmas)) {}

    // This type is movable.
    Task(Task&& other)
        : request(std::move(other.request)), dmas(std::move(other.dmas)) {}
    Task& operator=(Task&& other) {
      if (this != &other) {
        request = std::move(other.request);
        dmas = std::move(other.dmas);
      }
      return *this;
    }

    // Request.
    std::shared_ptr<Request> request;

    // DMAs to be performed to serve request. std::list is intentionally used to
    // have valid pointers while other members removed.
    std::list<DmaInfo> dmas;
  };

  // A data structure for keeping track of DMA and its associated request.
  struct PendingDma {
    // DMA.
    DmaInfo* info;

    // Related request.
    Request* request;
  };

  // Validates whether in "is_open" state.
  util::Status ValidateOpenState(bool is_open) const
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Locked version of IsEmpty().
  bool IsEmptyLocked() const EXCLUSIVE_LOCKS_REQUIRED(mutex_) {
    return pending_tasks_.empty() && active_tasks_.empty() &&
           pending_dmas_.empty();
  }

  // Handles all completed DMAs related cleanups for given tasks.
  util::Status HandleCompletedTasks() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  util::Status HandleActiveTasks() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Waits until all active DMAs are completed.
  util::Status CloseActiveDmas() LOCKS_EXCLUDED(mutex_);

  // Guards all the related queues.
  mutable std::mutex mutex_;

  // A notification to wait for all active requests to complete.
  std::condition_variable wait_active_requests_complete_;

  // A notification to wait for all active dmas to complete.
  std::condition_variable wait_active_dmas_complete_;

  // Tracks open state.
  bool is_open_ GUARDED_BY(mutex_){false};

  // Pending tasks that have not yet performed any DMAs to DarwiNN device.
  std::queue<Task> pending_tasks_ GUARDED_BY(mutex_);

  // Active tasks that have delivered DMAs fully or partially to DarwiNN device.
  std::queue<Task> active_tasks_ GUARDED_BY(mutex_);

  // Completed tasks that may have few active on-going DMAs.
  std::queue<Task> completed_tasks_ GUARDED_BY(mutex_);

  // DMAs belonging to active requests that are not yet served.
  std::queue<PendingDma> pending_dmas_ GUARDED_BY(mutex_);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_SINGLE_QUEUE_DMA_SCHEDULER_H_
