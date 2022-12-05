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
#include <functional>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "aemu/base/synchronization/Lock.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
#include "render-utils/virtio_gpu_ops.h"

struct VirtioGpuRingGlobal {};
struct VirtioGpuRingContextSpecific {
    VirtioGpuCtxId mCtxId;
    VirtioGpuRingIdx mRingIdx;
};
using VirtioGpuRing = std::variant<VirtioGpuRingGlobal, VirtioGpuRingContextSpecific>;

template <>
struct std::hash<VirtioGpuRingGlobal> {
    std::size_t operator()(VirtioGpuRingGlobal const&) const noexcept { return 0; }
};

inline bool operator==(const VirtioGpuRingGlobal&, const VirtioGpuRingGlobal&) { return true; }

template <>
struct std::hash<VirtioGpuRingContextSpecific> {
    std::size_t operator()(VirtioGpuRingContextSpecific const& ringContextSpecific) const noexcept {
        std::size_t ctxHash = std::hash<VirtioGpuCtxId>{}(ringContextSpecific.mCtxId);
        std::size_t ringHash = std::hash<VirtioGpuRingIdx>{}(ringContextSpecific.mRingIdx);
        // Use the hash_combine from
        // https://www.boost.org/doc/libs/1_78_0/boost/container_hash/hash.hpp.
        std::size_t res = ctxHash;
        res ^= ringHash + 0x9e3779b9 + (res << 6) + (res >> 2);
        return res;
    }
};

inline bool operator==(const VirtioGpuRingContextSpecific& lhs,
                       const VirtioGpuRingContextSpecific& rhs) {
    return lhs.mCtxId == rhs.mCtxId && lhs.mRingIdx == rhs.mRingIdx;
}

inline std::string to_string(const VirtioGpuRing& ring) {
    struct {
        std::string operator()(const VirtioGpuRingGlobal&) { return "global"; }
        std::string operator()(const VirtioGpuRingContextSpecific& ring) {
            std::stringstream ss;
            ss << "context specific {ctx = " << ring.mCtxId << ", ring = " << ring.mRingIdx << "}";
            return ss.str();
        }
    } visitor;
    return std::visit(visitor, ring);
}

class VirtioGpuTimelines {
   public:
    using FenceId = uint64_t;
    using Ring = VirtioGpuRing;
    using TaskId = uint64_t;

    TaskId enqueueTask(const Ring&);
    void enqueueFence(const Ring&, FenceId, FenceCompletionCallback);
    void notifyTaskCompletion(TaskId);
    void poll();
    std::vector<uint8_t> saveSnapshot();
    static std::unique_ptr<VirtioGpuTimelines> create(bool withAsyncCallback);
    using FenceCompletionCallbackGenerator =
        std::function<FenceCompletionCallback(const Ring&, FenceId)>;
    static std::unique_ptr<VirtioGpuTimelines> recreateFromSnapshot(
        std::unique_ptr<VirtioGpuTimelines> oldTimelines, const void* buffer,
        const FenceCompletionCallbackGenerator&);

   private:
    VirtioGpuTimelines(bool withAsyncCallback);
    struct Fence {
        FenceId mId;
        std::unique_ptr<FenceCompletionCallback> mCompletionCallback;
        Fence(FenceId id, FenceCompletionCallback completionCallback)
            : mId(id),
              mCompletionCallback(std::make_unique<FenceCompletionCallback>(completionCallback)) {}
    };
    struct Task {
        TaskId mId;
        Ring mRing;
        std::atomic_bool mHasCompleted;
        Task(TaskId id, const Ring& ring) : mId(id), mRing(ring), mHasCompleted(false) {}
    };
    using TimelineItem =
        std::variant<std::unique_ptr<Fence>, std::shared_ptr<Task>>;
    android::base::Lock mLock;
    std::atomic<TaskId> mNextId;
    // The mTaskIdToTask cache must be destroyed after the actual owner of Task,
    // mTimelineQueues, is destroyed, because the deleter of Task will
    // automatically remove the entry in mTaskIdToTask.
    std::unordered_map<TaskId, std::weak_ptr<Task>> mTaskIdToTask;
    std::unordered_map<Ring, std::list<TimelineItem>> mTimelineQueues;
    const bool mWithAsyncCallback;
    // Go over the timeline, signal any fences without pending tasks, and remove
    // timeline items that are no longer needed.
    void poll_locked(const Ring&);
};

#endif  // VIRTIO_GPU_TIMELINES_H
