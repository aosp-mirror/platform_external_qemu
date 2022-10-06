/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "aemu/base/HealthMonitor.h"

#include <map>

#include "aemu/base/system/System.h"
#include "host-common/logging.h"
#include "host-common/GfxstreamFatalError.h"
#include "testing/TestClock.h"

namespace emugl {

using android::base::AutoLock;
using android::base::MetricEventHang;
using android::base::MetricEventUnHang;
using android::base::TestClock;
using std::chrono::duration_cast;
using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;

template <class... Ts>
struct MonitoredEventVisitor : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
MonitoredEventVisitor(Ts...) -> MonitoredEventVisitor<Ts...>;

template <class Clock>
HealthMonitor<Clock>::HealthMonitor(MetricsLogger& metricsLogger, uint64_t heartbeatInterval)
    : mInterval(Duration(std::chrono::milliseconds(heartbeatInterval))), mLogger(metricsLogger) {
    start();
}

template <class Clock>
HealthMonitor<Clock>::~HealthMonitor() {
    auto event = std::make_unique<MonitoredEvent>(typename MonitoredEventType::EndMonitoring{});
    {
        AutoLock lock(mLock);
        mEventQueue.push(std::move(event));
    }
    poll();
    wait();
}

template <class Clock>
typename HealthMonitor<Clock>::Id HealthMonitor<Clock>::startMonitoringTask(
    std::unique_ptr<EventHangMetadata> metadata,
    std::optional<std::function<std::unique_ptr<HangAnnotations>()>> onHangAnnotationsCallback,
    uint64_t timeout, std::optional<Id> parentId) {
    auto intervalMs = duration_cast<std::chrono::milliseconds>(mInterval).count();
    if (timeout < intervalMs) {
        WARN("Timeout value %d is too low (heartbeat is every %d). Increasing to %d", timeout,
             intervalMs, intervalMs * 2);
        timeout = intervalMs * 2;
    }

    AutoLock lock(mLock);
    auto id = mNextId++;
    auto event = std::make_unique<MonitoredEvent>(typename MonitoredEventType::Start{
        .id = id,
        .metadata = std::move(metadata),
        .timeOccurred = Clock::now(),
        .onHangAnnotationsCallback = std::move(onHangAnnotationsCallback),
        .timeoutThreshold = Duration(std::chrono::milliseconds(timeout)),
        .parentId = parentId});
    mEventQueue.push(std::move(event));
    return id;
}

template <class Clock>
void HealthMonitor<Clock>::touchMonitoredTask(Id id) {
    auto event = std::make_unique<MonitoredEvent>(
        typename MonitoredEventType::Touch{.id = id, .timeOccurred = Clock::now()});
    AutoLock lock(mLock);
    mEventQueue.push(std::move(event));
}

template <class Clock>
void HealthMonitor<Clock>::stopMonitoringTask(Id id) {
    auto event = std::make_unique<MonitoredEvent>(
        typename MonitoredEventType::Stop{.id = id, .timeOccurred = Clock::now()});
    AutoLock lock(mLock);
    mEventQueue.push(std::move(event));
}

template <class Clock>
std::future<void> HealthMonitor<Clock>::poll() {
    auto event = std::make_unique<MonitoredEvent>(typename MonitoredEventType::Poll{});
    std::future<void> ret =
        std::get<typename MonitoredEventType::Poll>(*event).complete.get_future();

    AutoLock lock(mLock);
    mEventQueue.push(std::move(event));
    mCv.signalAndUnlock(&lock);
    return ret;
}

// Thread's main loop
template <class Clock>
intptr_t HealthMonitor<Clock>::main() {
    bool keepMonitoring = true;
    std::queue<std::unique_ptr<MonitoredEvent>> events;

    while (keepMonitoring) {
        std::vector<std::promise<void>> pollPromises;
        std::unordered_set<Id> tasksToRemove;
        int newHungTasks = mHungTasks;
        {
            AutoLock lock(mLock);
            if (mEventQueue.empty()) {
                mCv.timedWait(
                    &mLock,
                    android::base::getUnixTimeUs() +
                        std::chrono::duration_cast<std::chrono::microseconds>(mInterval).count());
            }
            mEventQueue.swap(events);
        }

        Timestamp now = Clock::now();
        while (!events.empty()) {
            auto event(std::move(events.front()));
            events.pop();

            std::visit(MonitoredEventVisitor{
                           [](std::monostate& event) {
                               ERR("MonitoredEvent type not found");
                               GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) <<
                                   "MonitoredEvent type not found";
                           },
                           [this, &events](typename MonitoredEventType::Start& event) {
                               auto it = mMonitoredTasks.find(event.id);
                               if (it != mMonitoredTasks.end()) {
                                   ERR("Registered multiple start events for task %d", event.id);
                                   return;
                               }
                               if (event.parentId && mMonitoredTasks.find(event.parentId.value()) ==
                                                         mMonitoredTasks.end()) {
                                   WARN("Requested parent task %d does not exist.",
                                        event.parentId.value());
                                   event.parentId = std::nullopt;
                               }
                               it = mMonitoredTasks
                                        .emplace(event.id,
                                                 std::move(MonitoredTask{
                                                 .id = event.id,
                                                     .timeoutTimestamp = event.timeOccurred +
                                                                         event.timeoutThreshold,
                                                 .timeoutThreshold = event.timeoutThreshold,
                                                 .hungTimestamp = std::nullopt,
                                                 .metadata = std::move(event.metadata),
                                                 .onHangAnnotationsCallback =
                                                         std::move(event.onHangAnnotationsCallback),
                                                     .parentId = event.parentId}))
                                        .first;
                               updateTaskParent(events, it->second, event.timeOccurred);
                           },
                           [this, &events](typename MonitoredEventType::Touch& event) {
                               auto it = mMonitoredTasks.find(event.id);
                               if (it == mMonitoredTasks.end()) {
                                   ERR("HealthMonitor has no task in progress for id %d", event.id);
                                   return;
                               }

                               auto& task = it->second;
                               task.timeoutTimestamp = event.timeOccurred + task.timeoutThreshold;
                               updateTaskParent(events, task, event.timeOccurred);
                           },
                           [this, &tasksToRemove,
                            &events](typename MonitoredEventType::Stop& event) {
                               auto it = mMonitoredTasks.find(event.id);
                               if (it == mMonitoredTasks.end()) {
                                   ERR("HealthMonitor has no task in progress for id %d", event.id);
                                   return;
                               }

                               auto& task = it->second;
                               task.timeoutTimestamp = event.timeOccurred + task.timeoutThreshold;
                               updateTaskParent(events, task, event.timeOccurred);

                               // Mark it for deletion, but retain it until the end of
                               // the health check concurrent tasks hung
                               tasksToRemove.insert(event.id);
                           },
                           [&keepMonitoring](typename MonitoredEventType::EndMonitoring& event) {
                               keepMonitoring = false;
                           },
                           [&pollPromises](typename MonitoredEventType::Poll& event) {
                               pollPromises.push_back(std::move(event.complete));
                           }},
                       *event);
        }

        // Sort by what times out first. Identical timestamps are possible
        std::multimap<Timestamp, uint64_t> sortedTasks;
        for (auto& [_, task] : mMonitoredTasks) {
            sortedTasks.insert(std::pair<Timestamp, uint64_t>(task.timeoutTimestamp, task.id));
        }

        for (auto& [_, task_id] : sortedTasks) {
            auto& task = mMonitoredTasks[task_id];
            if (task.timeoutTimestamp < now) {
                // Newly hung task
                if (!task.hungTimestamp.has_value()) {
                    // Copy over additional annotations captured at hangTime
                    if (task.onHangAnnotationsCallback) {
                        auto newAnnotations = (*task.onHangAnnotationsCallback)();
                        task.metadata->mergeAnnotations(std::move(newAnnotations));
                    }
                    mLogger.logMetricEvent(MetricEventHang{.taskId = task.id,
                                                           .metadata = task.metadata.get(),
                                                           .otherHungTasks = newHungTasks});
                    task.hungTimestamp = task.timeoutTimestamp;
                    newHungTasks++;
                }
            } else {
                // Task resumes
                if (task.hungTimestamp.has_value()) {
                    auto hangTime = duration_cast<std::chrono::milliseconds>(
                                        task.timeoutTimestamp -
                                        (task.hungTimestamp.value() + task.timeoutThreshold))
                                        .count();
                    mLogger.logMetricEvent(MetricEventUnHang{
                        .taskId = task.id, .metadata = task.metadata.get(), .hung_ms = hangTime});
                    task.hungTimestamp = std::nullopt;
                    newHungTasks--;
                }
            }
            if (tasksToRemove.find(task_id) != tasksToRemove.end()) {
                mMonitoredTasks.erase(task_id);
            }
        }

        if (mHungTasks != newHungTasks) {
            ERR("HealthMonitor: Number of unresponsive tasks %s: %d -> %d",
                mHungTasks < newHungTasks ? "increased" : "decreaased", mHungTasks, newHungTasks);
            mHungTasks = newHungTasks;
        }

        for (auto& complete : pollPromises) {
            complete.set_value();
        }
    }

    return 0;
}

template <class Clock>
void HealthMonitor<Clock>::updateTaskParent(std::queue<std::unique_ptr<MonitoredEvent>>& events,
                                            const MonitoredTask& task, Timestamp eventTime) {
    std::optional<Id> parentId = task.parentId;
    if (parentId) {
        auto event = std::make_unique<MonitoredEvent>(typename MonitoredEventType::Touch{
            .id = parentId.value(), .timeOccurred = eventTime + Duration(kTimeEpsilon)});
        events.push(std::move(event));
    }
}

template class HealthMonitor<steady_clock>;
template class HealthMonitor<TestClock>;

}  // namespace emugl
