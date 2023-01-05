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
#include "VirtioGpuTimelines_generated.h"
#include "host-common/GfxstreamFatalError.h"

#include <cinttypes>
#include <cstdio>
#include <type_traits>

using TaskId = VirtioGpuTimelines::TaskId;
using Ring = VirtioGpuTimelines::Ring;
using FenceId = VirtioGpuTimelines::FenceId;
using AutoLock = android::base::AutoLock;
using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;

std::unique_ptr<VirtioGpuTimelines> VirtioGpuTimelines::create(bool withAsyncCallback) {
    return std::unique_ptr<VirtioGpuTimelines>(new VirtioGpuTimelines(withAsyncCallback));
}

std::unique_ptr<VirtioGpuTimelines> VirtioGpuTimelines::recreateFromSnapshot(
    std::unique_ptr<VirtioGpuTimelines> oldTimelines, const void* buffer,
    const FenceCompletionCallbackGenerator& callbackGenerator) {
    std::unique_ptr<VirtioGpuTimelines> res = create(oldTimelines->mWithAsyncCallback);
    std::unique_ptr<virtio_gpu::TimelinesT> timelines =
        virtio_gpu::UnPackSizePrefixedTimelines(buffer);
    for (const std::unique_ptr<virtio_gpu::TimelineT>& timeline : timelines->timelines) {
        Ring ring = VirtioGpuRingGlobal{};
        if (timeline->ring.type == virtio_gpu::Ring::RingContextSpecific) {
            auto ringContextSpecific = *timeline->ring.AsRingContextSpecific();
            ring = VirtioGpuRingContextSpecific{
                .mCtxId = ringContextSpecific.ctx_id(),
                .mRingIdx = ringContextSpecific.ring_idx(),
            };
        }
        for (const virtio_gpu::Fence& fence : timeline->fences) {
            auto callback = callbackGenerator(ring, fence.id());
            res->enqueueFence(ring, fence.id(), std::move(callback));
        }
    }
    return res;
}

VirtioGpuTimelines::VirtioGpuTimelines(bool withAsyncCallback)
    : mNextId(0), mWithAsyncCallback(withAsyncCallback) {}

TaskId VirtioGpuTimelines::enqueueTask(const Ring& ring) {
    AutoLock lock(mLock);

    TaskId id = mNextId++;
    std::shared_ptr<Task> task(new Task(id, ring), [this](Task* task) {
        mTaskIdToTask.erase(task->mId);
        delete task;
    });
    mTaskIdToTask[id] = task;
    mTimelineQueues[ring].emplace_back(std::move(task));
    return id;
}

void VirtioGpuTimelines::enqueueFence(const Ring& ring, FenceId id,
                                      FenceCompletionCallback fenceCompletionCallback) {
    AutoLock lock(mLock);

    auto fence = std::make_unique<Fence>(id, fenceCompletionCallback);
    mTimelineQueues[ring].emplace_back(std::move(fence));
    if (mWithAsyncCallback) {
        poll_locked(ring);
    }
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
    if (mWithAsyncCallback) {
        poll_locked(task->mRing);
    }
}

void VirtioGpuTimelines::poll() {
    if (mWithAsyncCallback) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Can't call poll with async callback enabled.";
    }
    AutoLock lock(mLock);
    for (const auto& [ring, timeline] : mTimelineQueues) {
        poll_locked(ring);
    }
}
void VirtioGpuTimelines::poll_locked(const Ring& ring) {
    auto iTimelineQueue = mTimelineQueues.find(ring);
    if (iTimelineQueue == mTimelineQueues.end()) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Ring(" << to_string(ring) << ") doesn't exist.";
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

std::vector<uint8_t> VirtioGpuTimelines::saveSnapshot() {
    flatbuffers::FlatBufferBuilder builder;
    std::vector<flatbuffers::Offset<virtio_gpu::Timeline>> timelines;
    for (const auto& [ring, timeline] : mTimelineQueues) {
        std::vector<virtio_gpu::Fence> fences;
        for (const auto& timelineItem : timeline) {
            std::visit(
                [&fences](const auto& item) -> void {
                    using ItemType = std::decay_t<decltype(item)>;
                    if constexpr (std::is_same_v<
                                      typename std::pointer_traits<ItemType>::element_type,
                                      Fence>) {
                        fences.emplace_back(virtio_gpu::Fence(item->mId));
                    }
                },
                timelineItem);
        }
        auto [resultRingType, resultRing] = std::visit(
            [&builder](const auto& ring) {
                using RingType = std::decay_t<decltype(ring)>;
                if constexpr (std::is_same_v<RingType, VirtioGpuRingGlobal>) {
                    return std::make_tuple(virtio_gpu::Ring::RingGlobal,
                                           virtio_gpu::CreateRingGlobal(builder).Union());
                } else if constexpr (std::is_same_v<RingType, VirtioGpuRingContextSpecific>) {
                    auto resultRing = builder.CreateStruct(
                        virtio_gpu::RingContextSpecific(ring.mCtxId, ring.mRingIdx));
                    return std::make_tuple(virtio_gpu::Ring::RingContextSpecific,
                                           resultRing.Union());
                }
            },
            ring);
        auto resultFences = builder.CreateVectorOfStructs(fences);
        auto resultTimeline = CreateTimeline(builder, resultRingType, resultRing, resultFences);
        timelines.emplace_back(resultTimeline);
    }
    auto resultTimelines = builder.CreateVector(timelines);
    builder.FinishSizePrefixed(CreateTimelines(builder, resultTimelines));
    return std::vector(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
}
