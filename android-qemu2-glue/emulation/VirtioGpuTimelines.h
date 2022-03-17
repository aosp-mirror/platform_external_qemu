// Copyright 2021 The Android Open Source Project
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
// limitations under the License.
#ifndef VIRTIO_GPU_TIMELINES_H
#define VIRTIO_GPU_TIMELINES_H

#include <atomic>
#include <list>
#include <memory>
#include <unordered_map>
#include <variant>

#include "android/base/synchronization/Lock.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
#include "android/opengl/virtio_gpu_ops.h"

class VirtioGpuTimelines {
   public:
    using FenceId = uint64_t;
    using CtxId = VirtioGpuCtxId;
    using TaskId = uint64_t;
    VirtioGpuTimelines();

    TaskId enqueueTask(CtxId);
    void enqueueFence(CtxId, FenceId, FenceCompletionCallback);
    void notifyTaskCompletion(TaskId);

   private:
    struct Fence {
        std::unique_ptr<FenceCompletionCallback> mCompletionCallback;
        Fence(FenceCompletionCallback completionCallback)
            : mCompletionCallback(std::make_unique<FenceCompletionCallback>(
                  completionCallback)) {}
    };
    struct Task {
        TaskId mId;
        CtxId mCtxId;
        std::atomic_bool mHasCompleted;
        Task(TaskId id, CtxId ctxId)
            : mId(id), mCtxId(ctxId), mHasCompleted(false) {}
    };
    using TimelineItem =
        std::variant<std::unique_ptr<Fence>, std::shared_ptr<Task>>;
    android::base::Lock mLock;
    std::atomic<TaskId> mNextId;
    // The mTaskIdToTask cache must be destroyed after the actual owner of Task,
    // mTimelineQueues, is destroyed, because the deleter of Task will
    // automatically remove the entry in mTaskIdToTask.
    std::unordered_map<TaskId, std::weak_ptr<Task>> mTaskIdToTask;
    std::unordered_map<CtxId, std::list<TimelineItem>> mTimelineQueues;
    // Go over the timeline, signal any fences without pending tasks, and remove
    // timeline items that are no longer needed.
    void poll_locked(CtxId);
};

#endif  // VIRTIO_GPU_TIMELINES_H
