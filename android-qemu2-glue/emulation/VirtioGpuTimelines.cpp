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
#include "VirtioGpuTimelines.h"
#include "android/utils/GfxstreamFatalError.h"

#include <cinttypes>
#include <cstdio>

using TaskId = VirtioGpuTimelines::TaskId;
using CtxId = VirtioGpuTimelines::CtxId;
using FenceId = VirtioGpuTimelines::FenceId;
using AutoLock = android::base::AutoLock;

VirtioGpuTimelines::VirtioGpuTimelines() : mNextId(0) {}

TaskId VirtioGpuTimelines::enqueueTask(CtxId ctxId) {
    AutoLock lock(mLock);

    TaskId id = mNextId++;
    auto task = std::make_shared<Task>(id, ctxId);
    mTaskIdToTask[id] = task;
    mTimelineQueues[ctxId].emplace_back(std::move(task));
    return id;
}

void VirtioGpuTimelines::enqueueFence(
    CtxId ctxId, FenceId, FenceCompletionCallback fenceCompletionCallback) {
    AutoLock lock(mLock);

    auto fence = std::make_unique<Fence>(fenceCompletionCallback);
    mTimelineQueues[ctxId].emplace_back(std::move(fence));
    poll_locked(ctxId);
}

void VirtioGpuTimelines::notifyTaskCompletion(TaskId taskId) {
    AutoLock lock(mLock);
    auto iTask = mTaskIdToTask.find(taskId);
    if (iTask == mTaskIdToTask.end()) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Task(id = " << static_cast<uint64_t>(taskId) << ") can't be found";
    }
    std::shared_ptr<Task> task = iTask->second.lock();
    if (task == nullptr) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Task(id = " << static_cast<uint64_t>(taskId) << ") has been destroyed";
    }
    if (task->mId != taskId) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Task id mismatch. Expected " << static_cast<uint64_t>(taskId) << " Actual "
            << static_cast<uint64_t>(task->mId);
    }
    if (task->mHasCompleted) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Task(id = " << static_cast<uint64_t>(taskId) << ") has been set to completed.";
    }
    task->mHasCompleted = true;
    poll_locked(task->mCtxId);
}

void VirtioGpuTimelines::poll_locked(CtxId ctxId) {
    auto iTimelineQueue = mTimelineQueues.find(ctxId);
    if (iTimelineQueue == mTimelineQueues.end()) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Context(id = " << ctxId << " doesn't exist";
    }
    std::list<TimelineItem> &timelineQueue = iTimelineQueue->second;
    auto i = timelineQueue.begin();
    for (; i != timelineQueue.end(); i++) {
        // This visitor will signal the fence and return whether the timeline
        // item is an incompleted task.
        struct {
            bool operator()(std::unique_ptr<Fence> &fence) {
                (*fence->mCompletionCallback)();
                return false;
            }
            bool operator()(std::shared_ptr<Task> &task) {
                return !task->mHasCompleted;
            }
        } visitor;
        if (std::visit(visitor, *i)) {
            break;
        }
    }
    timelineQueue.erase(timelineQueue.begin(), i);
}