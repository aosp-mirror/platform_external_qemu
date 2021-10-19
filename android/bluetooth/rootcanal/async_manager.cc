/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "model/setup/async_manager.h"  // for AsyncManager

#include <errno.h>                              // for errno
#include <atomic>                               // for atomic_bool, atomic_e...
#include <condition_variable>                   // for condition_variable
#include <cstring>                              // for strerror
#include <limits>                               // for numeric_limits
#include <map>                                  // for map<>::value_type, map
#include <mutex>                                // for unique_lock, mutex
#include <ratio>                                // for ratio
#include <set>                                  // for set
#include <thread>                               // for thread
#include <type_traits>                          // for remove_extent_t
#include <utility>                              // for pair, make_pair, oper...
#include <vector>                               // for vector

#include "android/base/EintrWrapper.h"          // for HANDLE_EINTR
#include "android/base/Log.h"                   // for LogStreamVoidify, Log...
#include "android/base/sockets/SocketUtils.h"   // for socketRecv, socketSet...
#include "android/base/sockets/SocketWaiter.h"  // for SocketWaiter, SocketW...
#include "android/utils/debug.h"

namespace test_vendor_lib {
// Implementation of AsyncManager is divided between two classes, three if
// AsyncManager itself is taken into account, but its only responsability
// besides being a proxy for the other two classes is to provide a global
// synchronization mechanism for callbacks and client code to use.

// The watching of file descriptors is done through AsyncFdWatcher. Several
// objects of this class may coexist simultaneosly as they share no state.
// After construction of this objects nothing happens beyond some very simple
// member initialization. When the first FD is set up for watching the object
// starts a new thread which watches the given (and later provided) FDs using
// select() inside a loop. A special FD (a pipe) is also watched which is
// used to notify the thread of internal changes on the object state (like
// the addition of new FDs to watch on). Every access to internal state is
// synchronized using a single internal mutex. The thread is only stopped on
// destruction of the object, by modifying a flag, which is the only member
// variable accessed without acquiring the lock (because the notification to
// the thread is done later by writing to a pipe which means the thread will
// be notified regardless of what phase of the loop it is in that moment)

// The scheduling of asynchronous tasks, periodic or not, is handled by the
// AsyncTaskManager class. Like the one for FDs, this class shares no internal
// state between different instances so it is safe to use several objects of
// this class, also nothing interesting happens upon construction, but only
// after a Task has been scheduled and access to internal state is synchronized
// using a single internal mutex. When the first task is scheduled a thread
// is started which monitors a queue of tasks. The queue is peeked to see
// when the next task should be carried out and then the thread performs a
// (absolute) timed wait on a condition variable. The wait ends because of a
// time out or a notify on the cond var, the former means a task is due
// for execution while the later means there has been a change in internal
// state, like a task has been scheduled/canceled or the flag to stop has
// been set. Setting and querying the stop flag or modifying the task queue
// and subsequent notification on the cond var is done atomically (e.g while
// holding the lock on the internal mutex) to ensure that the thread never
// misses the notification, since notifying a cond var is not persistent as
// writing on a pipe (if not done this way, the thread could query the
// stopping flag and be put aside by the OS scheduler right after, then the
// 'stop thread' procedure could run, setting the flag, notifying a cond
// var that no one is waiting on and joining the thread, the thread then
// resumes execution believing that it needs to continue and waits on the
// cond var possibly forever if there are no tasks scheduled, efectively
// causing a deadlock).

// This number also states the maximum number of scheduled tasks we can handle
// at a given time
static const uint16_t kMaxTaskId = -1; /* 2^16 - 1, permisible ids are {1..2^16-1}*/
static inline AsyncTaskId NextAsyncTaskId(const AsyncTaskId id) {
  return (id == kMaxTaskId) ? 1 : id + 1;
}
// The buffer is only 10 bytes because the expected number of bytes
// written on this socket is 1. It is possible that the thread is notified
// more than once but highly unlikely, so a buffer of size 10 seems enough
// and the reads are performed inside a while just in case it isn't. From
// the thread routine's point of view it is the same to have been notified
// just once or 100 times so it just tries to consume the entire buffer.
// In the cases where an interrupt would cause read to return without
// having read everything that was available a new iteration of the thread
// loop will bring execution to this point almost immediately, so there is
// no need to treat that case.
static const int kNotificationBufferSize = 10;


using android::base::SocketWaiter;

// Async File Descriptor Watcher Implementation:
class AsyncManager::AsyncFdWatcher {

 public:
  int WatchFdForNonBlockingReads(int file_descriptor, const ReadCallback& on_read_fd_ready_callback) {
    // add file descriptor and callback
    {
      std::unique_lock<std::recursive_mutex> guard(internal_mutex_);
      watched_shared_fds_[file_descriptor] = on_read_fd_ready_callback;
    }

    // start the thread if not started yet
    int started = tryStartThread();
    if (started != 0) {
      derror("%s: Unable to start thread", __func__);
      return started;
    }

    // notify the thread so that it knows of the new FD
    notifyThread();

    return 0;
  }

  void StopWatchingFileDescriptor(int file_descriptor) {
    std::unique_lock<std::recursive_mutex> guard(internal_mutex_);
    watched_shared_fds_.erase(file_descriptor);
  }

  AsyncFdWatcher() = default;
  AsyncFdWatcher(const AsyncFdWatcher&) = delete;
  AsyncFdWatcher& operator=(const AsyncFdWatcher&) = delete;

  ~AsyncFdWatcher() = default;

  int stopThread() {
    if (!std::atomic_exchange(&running_, false)) {
      return 0;  // if not running already
    }

    notifyThread();

    if (std::this_thread::get_id() != thread_.get_id()) {
      thread_.join();
    } else {
      dwarning("%s: Starting thread stop from inside the reading thread itself", __func__);
    }

    {
      std::unique_lock<std::recursive_mutex> guard(internal_mutex_);
      watched_shared_fds_.clear();
    }

    return 0;
  }

 private:
  // Make sure to call this with at least one file descriptor ready to be
  // watched upon or the thread routine will return immediately
  int tryStartThread() {
    if (std::atomic_exchange(&running_, true)) {
      return 0;  // if already running
    }
    // set up the communication channel
    if (android::base::socketCreatePair(&notification_listen_fd_, &notification_write_fd_)) {
      derror(
          "%s:Unable to establish a communication channel to the reading "
          "thread",
          __func__);
      return -1;
    }
    android::base::socketSetNonBlocking(notification_listen_fd_);
    android::base::socketSetNonBlocking(notification_write_fd_);

    thread_ = std::thread([this]() { ThreadRoutine(); });
    if (!thread_.joinable()) {
      derror("%s: Unable to start reading thread", __func__);
      return -1;
    }
    return 0;
  }

  int notifyThread() {
    char buffer = '0';
    if (android::base::socketSend(notification_write_fd_, &buffer, 1) < 0) {
      derror("%s: Unable to send message to reading thread", __func__);
      return -1;
    }
    return 0;
  }

  void setUpFileDescriptorSet(SocketWaiter* read_fds) {
    // add comm channel to the set
    read_fds->update(notification_listen_fd_, SocketWaiter::Event::kEventRead);

    // add watched FDs to the set
    {
      std::unique_lock<std::recursive_mutex> guard(internal_mutex_);
      for (auto& fdp : watched_shared_fds_) {
        read_fds->update(fdp.first, SocketWaiter::Event::kEventRead);
      }
    }
  }

  // check the comm channel and read everything there
  bool consumeThreadNotifications(SocketWaiter* read_fds) {
    if (read_fds->pendingEventsFor(notification_listen_fd_)) {
      char buffer[kNotificationBufferSize];
      while (HANDLE_EINTR(android::base::socketRecv(notification_listen_fd_, buffer, kNotificationBufferSize)) ==
             kNotificationBufferSize) {
      }
      return true;
    }
    return false;
  }

  // check all file descriptors and call callbacks if necesary
  void runAppropriateCallbacks(SocketWaiter* read_fds) {
    // not a good idea to call a callback while holding the FD lock,
    // nor to release the lock while traversing the map
    std::vector<decltype(watched_shared_fds_)::value_type> fds;
    std::unique_lock<std::recursive_mutex> guard(internal_mutex_);
    for (auto& fdc : watched_shared_fds_) {
      auto pending = read_fds->pendingEventsFor(fdc.first);
      if (pending == SocketWaiter::kEventRead) {
        fds.push_back(fdc);
      }
    }

    for (auto& p : fds) {
      p.second(p.first);
    }

  }

  void ThreadRoutine() {
    auto read_fds = std::unique_ptr<SocketWaiter>(SocketWaiter::create());
    while (running_) {
      read_fds->reset();
      setUpFileDescriptorSet(read_fds.get());

      // wait until there is data available to read on some FD
      int retval = read_fds->wait(std::numeric_limits<int64_t>::max());
      if (retval <= 0) {  // there was some error or a timeout
        derror(
            "%s: There was an error while waiting for data on the file "
            "descriptors: %s",
            __func__, strerror(errno));
        continue;
      }

      consumeThreadNotifications(read_fds.get());

      // Do not read if there was a call to stop running
      if (!running_) {
        break;
      }

      runAppropriateCallbacks(read_fds.get());
    }
  }

  std::atomic_bool running_{false};
  std::thread thread_;
  std::recursive_mutex internal_mutex_;


  //android::base::SocketWaiter socket_waiter_;
  std::map<int, ReadCallback> watched_shared_fds_;

  // A pair of FD to send information to the reading thread
  int notification_listen_fd_{};
  int notification_write_fd_{};
};

// Async task manager implementation
class AsyncManager::AsyncTaskManager {
 public:
  AsyncUserId GetNextUserId() { return lastUserId_++; }

  AsyncTaskId ExecAsync(AsyncUserId user_id, std::chrono::milliseconds delay,
                        const TaskCallback& callback) {
    return scheduleTask(std::make_shared<Task>(
        std::chrono::steady_clock::now() + delay, callback, user_id));
  }

  AsyncTaskId ExecAsyncPeriodically(AsyncUserId user_id,
                                    std::chrono::milliseconds delay,
                                    std::chrono::milliseconds period,
                                    const TaskCallback& callback) {
    return scheduleTask(std::make_shared<Task>(
        std::chrono::steady_clock::now() + delay, period, callback, user_id));
  }

  bool CancelAsyncTask(AsyncTaskId async_task_id) {
    // remove task from queue (and task id association) while holding lock
    std::unique_lock<std::mutex> guard(internal_mutex_);
    return cancel_task_with_lock_held(async_task_id);
  }

  bool CancelAsyncTasksFromUser(AsyncUserId user_id) {
    // remove task from queue (and task id association) while holding lock
    std::unique_lock<std::mutex> guard(internal_mutex_);
    if (tasks_by_user_id_.count(user_id) == 0) {
      return false;
    }
    for (auto task : tasks_by_user_id_[user_id]) {
      cancel_task_with_lock_held(task);
    }
    tasks_by_user_id_.erase(user_id);
    return true;
  }

  AsyncTaskManager() = default;
  AsyncTaskManager(const AsyncTaskManager&) = delete;
  AsyncTaskManager& operator=(const AsyncTaskManager&) = delete;

  ~AsyncTaskManager() = default;

  int stopThread() {
    {
      std::unique_lock<std::mutex> guard(internal_mutex_);
      tasks_by_id_.clear();
      task_queue_.clear();
      if (!running_) {
        return 0;
      }
      running_ = false;
      // notify the thread
      internal_cond_var_.notify_one();
    }  // release the lock before joining a thread that is likely waiting for it
    if (std::this_thread::get_id() != thread_.get_id()) {
      thread_.join();
    } else {
      dwarning("%s: Starting thread stop from inside the task thread itself", __func__);
    }
    return 0;
  }

 private:
  // Holds the data for each task
  class Task {
   public:
    Task(std::chrono::steady_clock::time_point time,
         std::chrono::milliseconds period, const TaskCallback& callback,
         AsyncUserId user)
        : time(time),
          periodic(true),
          period(period),
          callback(callback),
          task_id(kInvalidTaskId),
          user_id(user) {}
    Task(std::chrono::steady_clock::time_point time,
         const TaskCallback& callback, AsyncUserId user)
        : time(time),
          periodic(false),
          callback(callback),
          task_id(kInvalidTaskId),
          user_id(user) {}

    // Operators needed to be in a collection
    bool operator<(const Task& another) const {
      return std::make_pair(time, task_id) < std::make_pair(another.time, another.task_id);
    }

    bool isPeriodic() const {
      return periodic;
    }

    // These fields should no longer be public if the class ever becomes
    // public or gets more complex
    std::chrono::steady_clock::time_point time;
    bool periodic;
    std::chrono::milliseconds period{};
    TaskCallback callback;
    AsyncTaskId task_id;
    AsyncUserId user_id;
  };

  // A comparator class to put shared pointers to tasks in an ordered set
  struct task_p_comparator {
    bool operator()(const std::shared_ptr<Task>& t1, const std::shared_ptr<Task>& t2) const {
      return *t1 < *t2;
    }
  };

  bool cancel_task_with_lock_held(AsyncTaskId async_task_id) {
    if (tasks_by_id_.count(async_task_id) == 0) {
      return false;
    }
    task_queue_.erase(tasks_by_id_[async_task_id]);
    tasks_by_id_.erase(async_task_id);
    return true;
  }

  AsyncTaskId scheduleTask(const std::shared_ptr<Task>& task) {
    {
      std::unique_lock<std::mutex> guard(internal_mutex_);
      // no more room for new tasks, we need a larger type for IDs
      if (tasks_by_id_.size() == kMaxTaskId)  // TODO potentially type unsafe
        return kInvalidTaskId;
      do {
        lastTaskId_ = NextAsyncTaskId(lastTaskId_);
      } while (isTaskIdInUse(lastTaskId_));
      task->task_id = lastTaskId_;
      // add task to the queue and map
      tasks_by_id_[lastTaskId_] = task;
      tasks_by_user_id_[task->user_id].insert(task->task_id);
      task_queue_.insert(task);
    }
    // start thread if necessary
    int started = tryStartThread();
    if (started != 0) {
      derror("%s: Unable to start thread", __func__);
      return kInvalidTaskId;
    }
    // notify the thread so that it knows of the new task
    internal_cond_var_.notify_one();
    // return task id
    return task->task_id;
  }

  bool isTaskIdInUse(const AsyncTaskId& task_id) const {
    return tasks_by_id_.count(task_id) != 0;
  }

  int tryStartThread() {
    // need the lock because of the running flag and the cond var
    std::unique_lock<std::mutex> guard(internal_mutex_);
    // check that the thread is not yet running
    if (running_) {
      return 0;
    }
    // start the thread
    running_ = true;
    thread_ = std::thread([this]() { ThreadRoutine(); });
    if (!thread_.joinable()) {
      derror("%s: Unable to start task thread", __func__);
      return -1;
    }
    return 0;
  }

  void ThreadRoutine() {
    while (running_) {
      TaskCallback callback;
      bool run_it = false;
      {
        std::unique_lock<std::mutex> guard(internal_mutex_);
        if (!task_queue_.empty()) {
          std::shared_ptr<Task> task_p = *(task_queue_.begin());
          if (task_p->time < std::chrono::steady_clock::now()) {
            run_it = true;
            callback = task_p->callback;
            task_queue_.erase(task_p);  // need to remove and add again if
                                        // periodic to update order
            if (task_p->isPeriodic()) {
              task_p->time += task_p->period;
              task_queue_.insert(task_p);
            } else {
              tasks_by_user_id_[task_p->user_id].erase(task_p->task_id);
              tasks_by_id_.erase(task_p->task_id);
            }
          }
        }
      }
      if (run_it) {
        callback();
      }
      {
        std::unique_lock<std::mutex> guard(internal_mutex_);
        // check for termination right before waiting
        if (!running_) break;
        // wait until time for the next task (if any)
        if (task_queue_.size() > 0) {
          // Make a copy of the time_point because wait_until takes a reference
          // to it and may read it after waiting, by which time the task may
          // have been freed (e.g. via CancelAsyncTask).
          std::chrono::steady_clock::time_point time =
              (*task_queue_.begin())->time;
          internal_cond_var_.wait_until(guard, time);
        } else {
          internal_cond_var_.wait(guard);
        }
      }
    }
  }

  bool running_ = false;
  std::thread thread_;
  std::mutex internal_mutex_;
  std::condition_variable internal_cond_var_;

  AsyncTaskId lastTaskId_ = kInvalidTaskId;
  AsyncUserId lastUserId_{1};
  std::map<AsyncTaskId, std::shared_ptr<Task> > tasks_by_id_;
  std::map<AsyncUserId, std::set<AsyncTaskId>> tasks_by_user_id_;
  std::set<std::shared_ptr<Task>, task_p_comparator> task_queue_;
};

// Async Manager Implementation:
AsyncManager::AsyncManager() : fdWatcher_p_(new AsyncFdWatcher()), taskManager_p_(new AsyncTaskManager()) {}

AsyncManager::~AsyncManager() {
  // Make sure the threads are stopped before destroying the object.
  // The threads need to be stopped here and not in each internal class'
  // destructor because unique_ptr's reset() first assigns nullptr to the
  // pointer and only then calls the destructor, so any callback running
  // on these threads would dereference a null pointer if they called a member
  // function of this class.
  fdWatcher_p_->stopThread();
  taskManager_p_->stopThread();
}

int AsyncManager::WatchFdForNonBlockingReads(int file_descriptor, const ReadCallback& on_read_fd_ready_callback) {
  return fdWatcher_p_->WatchFdForNonBlockingReads(file_descriptor, on_read_fd_ready_callback);
}

void AsyncManager::StopWatchingFileDescriptor(int file_descriptor) {
  fdWatcher_p_->StopWatchingFileDescriptor(file_descriptor);
}

AsyncUserId AsyncManager::GetNextUserId() {
  return taskManager_p_->GetNextUserId();
}

AsyncTaskId AsyncManager::ExecAsync(AsyncUserId user_id,
                                    std::chrono::milliseconds delay,
                                    const TaskCallback& callback) {
  return taskManager_p_->ExecAsync(user_id, delay, callback);
}

AsyncTaskId AsyncManager::ExecAsyncPeriodically(
    AsyncUserId user_id, std::chrono::milliseconds delay,
    std::chrono::milliseconds period, const TaskCallback& callback) {
  return taskManager_p_->ExecAsyncPeriodically(user_id, delay, period,
                                               callback);
}

bool AsyncManager::CancelAsyncTask(AsyncTaskId async_task_id) {
  return taskManager_p_->CancelAsyncTask(async_task_id);
}

bool AsyncManager::CancelAsyncTasksFromUser(
    test_vendor_lib::AsyncUserId user_id) {
  return taskManager_p_->CancelAsyncTasksFromUser(user_id);
}

void AsyncManager::Synchronize(const CriticalCallback& critical) {
  std::unique_lock<std::mutex> guard(synchronization_mutex_);
  critical();
}
}  // namespace test_vendor_lib
