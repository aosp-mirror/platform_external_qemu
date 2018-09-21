#include "third_party/darwinn/driver/single_queue_dma_scheduler.h"

#include <string>
#include <utility>

#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

util::Status SingleQueueDmaScheduler::ValidateOpenState(bool is_open) const {
  if (is_open_ != is_open) {
    return util::FailedPreconditionError(
        StringPrintf("Bad state: expected=%d, actual=%d", is_open, is_open_));
  }
  return util::Status();  // OK
}

util::Status SingleQueueDmaScheduler::Open() {
  StdMutexLock lock(&mutex_);
  if (!IsEmptyLocked()) {
    return util::FailedPreconditionError("DMA queues are not empty");
  }
  RETURN_IF_ERROR(ValidateOpenState(/*is_open=*/false));
  is_open_ = true;
  return util::Status();  // OK
}

util::Status SingleQueueDmaScheduler::Close() {
  {
    StdMutexLock lock(&mutex_);
    RETURN_IF_ERROR(ValidateOpenState(/*is_open=*/true));
    while (!pending_dmas_.empty()) {
      pending_dmas_.pop();
    }
  }

  util::Status status;
  status.Update(CancelPendingRequests());
  status.Update(CloseActiveDmas());

  StdMutexLock lock(&mutex_);
  is_open_ = false;
  return status;
}

util::Status SingleQueueDmaScheduler::Submit(std::shared_ptr<Request> request) {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateOpenState(/*is_open=*/true));

  RETURN_IF_ERROR(request->NotifyRequestSubmitted());
  VLOG(3) << StringPrintf("Request[%d]: Submitted", request->id());
  ASSIGN_OR_RETURN(auto dmas, request->GetDmaInfos());
  pending_tasks_.push({std::move(request), std::move(dmas)});

  return util::Status();  // OK
}

DmaDescriptorType SingleQueueDmaScheduler::PeekNextDma() const {
  StdMutexLock lock(&mutex_);
  CHECK_OK(ValidateOpenState(/*is_open=*/true));
  if (pending_dmas_.empty() && pending_tasks_.empty()) {
    return DmaDescriptorType::kLocalFence;
  }

  if (pending_dmas_.empty()) {
    return pending_tasks_.front().dmas.front().type();
  } else {
    return pending_dmas_.front().info->type();
  }
}

DmaInfo* SingleQueueDmaScheduler::GetNextDma() {
  StdMutexLock lock(&mutex_);
  CHECK_OK(ValidateOpenState(/*is_open=*/true));
  if (pending_dmas_.empty() && pending_tasks_.empty()) {
    return nullptr;
  }

  if (pending_dmas_.empty()) {
    auto& task = pending_tasks_.front();
    CHECK_OK(task.request->NotifyRequestActive());
    Request* request = task.request.get();
    for (auto& dma : task.dmas) {
      pending_dmas_.push({&dma, request});
    }
    active_tasks_.push(std::move(task));
    pending_tasks_.pop();
  }

  // If fenced, return empty DMAs.
  const auto& pending_front = pending_dmas_.front();
  if (pending_front.info->type() == DmaDescriptorType::kLocalFence ||
      pending_front.info->type() == DmaDescriptorType::kGlobalFence) {
    return nullptr;
  }

  pending_front.info->MarkActive();
  VLOG(7) << StringPrintf("Request[%d]: Scheduling DMA[%d]",
                          pending_front.request->id(),
                          pending_front.info->id());

  auto* next_dma = pending_front.info;
  pending_dmas_.pop();
  return next_dma;
}

util::Status SingleQueueDmaScheduler::NotifyDmaCompletion(DmaInfo* dma_info) {
  if (!dma_info->IsActive()) {
    const auto dma_dump = dma_info->Dump();
    return util::FailedPreconditionError(
        StringPrintf("Cannot complete inactive DMA: %s", dma_dump.c_str()));
  }

  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateOpenState(/*is_open=*/true));

  dma_info->MarkCompleted();
  VLOG(7) << StringPrintf("Completing DMA[%d]", dma_info->id());

  RETURN_IF_ERROR(HandleCompletedTasks());
  wait_active_dmas_complete_.notify_all();
  if (pending_dmas_.empty()) {
    return util::Status();  // OK
  }

  const auto& pending_front = pending_dmas_.front();
  if (pending_front.info->type() != DmaDescriptorType::kLocalFence) {
    return util::Status();  // OK
  }

  // Clear local fence if completed.
  RETURN_IF_ERROR(HandleActiveTasks());
  if (pending_front.info->IsCompleted()) {
    VLOG(7) << StringPrintf("Request[%d]: Local fence done",
                            pending_front.request->id());
    pending_dmas_.pop();
  }
  return util::Status();  // OK
}

util::Status SingleQueueDmaScheduler::NotifyRequestCompletion() {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateOpenState(/*is_open=*/true));
  if (active_tasks_.empty()) {
    return util::FailedPreconditionError("No active request to complete");
  }

  // Requests are always handled in FIFO order.
  Request* completed_request = active_tasks_.front().request.get();
  if (!pending_dmas_.empty()) {
    const auto& pending_front = pending_dmas_.front();

    if (pending_front.request == completed_request) {
      // Clear global fence if exists.
      if (pending_front.info->type() == DmaDescriptorType::kGlobalFence) {
        VLOG(7) << StringPrintf("Request[%d]: Global fence done",
                                completed_request->id());
        pending_front.info->MarkCompleted();
        pending_dmas_.pop();
      } else {
        return util::FailedPreconditionError(
            StringPrintf("Request[%d] is completing while DMAs are pending.",
                         completed_request->id()));
      }
    }
  }

  RETURN_IF_ERROR(HandleActiveTasks());
  Task next_front = std::move(active_tasks_.front());
  active_tasks_.pop();
  if (next_front.dmas.empty() && completed_tasks_.empty()) {
    RETURN_IF_ERROR(completed_request->NotifyCompletion(util::Status()));
    VLOG(3) << StringPrintf("Request[%d]: Completed", completed_request->id());
    wait_active_requests_complete_.notify_all();
  } else {
    completed_tasks_.push(std::move(next_front));
  }
  return util::Status();  // OK
}

util::Status SingleQueueDmaScheduler::CancelPendingRequests() {
  util::Status status;

  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateOpenState(/*is_open=*/true));
  while (!pending_tasks_.empty()) {
    status.Update(pending_tasks_.front().request->Cancel());
    pending_tasks_.pop();
  }
  return status;
}

util::Status SingleQueueDmaScheduler::WaitActiveRequests() {
  StdCondMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateOpenState(/*is_open=*/true));
  while (!completed_tasks_.empty() || !active_tasks_.empty()) {
    VLOG(3) << StringPrintf("Waiting for %zd more active requests",
                            completed_tasks_.size() + active_tasks_.size());
    wait_active_requests_complete_.wait(lock);
  }
  return util::Status();  // OK
}

util::Status SingleQueueDmaScheduler::HandleCompletedTasks() {
  if (completed_tasks_.empty()) {
    return util::Status();  // OK
  }

  completed_tasks_.front().dmas.remove_if(
      [](const DmaInfo& dma_info) { return dma_info.IsCompleted(); });

  // Complete tasks, whose DMAs are all completed.
  while (completed_tasks_.front().dmas.empty()) {
    auto& front_task = completed_tasks_.front();
    RETURN_IF_ERROR(front_task.request->NotifyCompletion(util::Status()));
    VLOG(3) << StringPrintf("Request[%d]: Completed", front_task.request->id());
    completed_tasks_.pop();

    if (completed_tasks_.empty()) {
      wait_active_requests_complete_.notify_all();
      break;
    }

    completed_tasks_.front().dmas.remove_if(
        [](const DmaInfo& dma_info) { return dma_info.IsCompleted(); });
  }

  return util::Status();  // OK
}

util::Status SingleQueueDmaScheduler::HandleActiveTasks() {
  if (active_tasks_.empty()) {
    return util::Status();  // OK
  }

  auto& front_task = active_tasks_.front();
  front_task.dmas.remove_if(
      [](const DmaInfo& dma_info) { return dma_info.IsCompleted(); });

  if (front_task.dmas.empty()) {
    return util::Status();  // OK
  }

  auto& front_dma = front_task.dmas.front();
  // If first remaining DMA is local fence, mark it completed.
  if (front_dma.type() == DmaDescriptorType::kLocalFence) {
    front_dma.MarkCompleted();
  }
  return util::Status();  // OK
}

util::Status SingleQueueDmaScheduler::CloseActiveDmas() {
  StdCondMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateOpenState(/*is_open=*/true));
  while (!completed_tasks_.empty()) {
    completed_tasks_.front().dmas.remove_if(
        [](const DmaInfo& info) { return !info.IsActive(); });
    if (completed_tasks_.front().dmas.empty()) {
      completed_tasks_.pop();
    }

    if (completed_tasks_.empty()) {
      break;
    }
    wait_active_dmas_complete_.wait(lock);
  }
  while (!active_tasks_.empty()) {
    active_tasks_.front().dmas.remove_if(
        [](const DmaInfo& info) { return !info.IsActive(); });
    if (active_tasks_.front().dmas.empty()) {
      active_tasks_.pop();
    }

    if (active_tasks_.empty()) {
      break;
    }

    wait_active_dmas_complete_.wait(lock);
  }
  return util::Status();  // OK
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
