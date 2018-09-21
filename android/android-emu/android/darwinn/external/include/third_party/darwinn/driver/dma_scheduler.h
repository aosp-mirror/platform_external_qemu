#ifndef THIRD_PARTY_DARWINN_DRIVER_DMA_SCHEDULER_H_
#define THIRD_PARTY_DARWINN_DRIVER_DMA_SCHEDULER_H_

#include <memory>
#include <vector>

#include "third_party/darwinn/driver/dma_info.h"
#include "third_party/darwinn/driver/request.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Manages the processing order of DMAs from DarwiNN Request, and also keeps
// track of the requests. All implementation of DMA scheduler has to be
// thread-safe.
//
// Example usage:
//   DmaScheduler scheduler;
//   scheduler.Submit(request0);
//   scheduler.Submit(request1);
//   ...
//   const auto* dma = scheduler.GetNextDma();
//   // Handle DMA.
//   if DMA is completed:
//     scheduler.NotifyDmaCompletion(dma);
//   ...
//   // when Request is complete
//   scheduler.NotifyRequestCompletion();
class DmaScheduler {
 public:
  DmaScheduler() = default;

  // This class is neither copyable nor movable.
  DmaScheduler(const DmaScheduler&) = delete;
  DmaScheduler& operator=(const DmaScheduler&) = delete;

  virtual ~DmaScheduler() = default;

  // Opens/closes DMA scheduler.
  virtual util::Status Open() = 0;
  virtual util::Status Close() = 0;

  // Submits a request for execution on DarwiNN.
  virtual util::Status Submit(std::shared_ptr<Request> request) = 0;

  // Returns next DMA type to be performed. Returns kLocalFence if there is no
  // next DMA.
  virtual DmaDescriptorType PeekNextDma() const = 0;

  // Returns DMA to perform. If there is no DMA to perform, returns nullptr.
  // Target of pointers are internally maintained.
  // DmaScheduler::NotifyDmaCompletion is a contract that given pointer is no
  // longer used by external entity.
  virtual DmaInfo* GetNextDma() = 0;

  // Notifies that DMA for given "dma_info" has completed. Returns an error if
  // given "dma_info" cannot be completed.
  virtual util::Status NotifyDmaCompletion(DmaInfo* dma_info) = 0;

  // Notifies when request has been completed, and performs any necessary
  // cleanups.
  virtual util::Status NotifyRequestCompletion() = 0;

  // Cancels all the pending requests that has not been submitted to DarwiNN
  // device yet.
  virtual util::Status CancelPendingRequests() = 0;

  // Waits until active requests are done.
  virtual util::Status WaitActiveRequests() = 0;

  // Returns true if there is no DMAs to schedule.
  virtual bool IsEmpty() const = 0;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_DMA_SCHEDULER_H_
