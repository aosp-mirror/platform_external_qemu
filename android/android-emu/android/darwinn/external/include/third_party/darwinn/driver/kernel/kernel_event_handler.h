#ifndef THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_EVENT_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_EVENT_HANDLER_H_

#include <functional>
#include <memory>
#include <mutex>   // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Monitors events generated through eventfd. The eventfd file
// descriptor passed through the constructor must already be open
// and associated with an interrupt source. Monitoring starts
// on instance creation and stops on destroy.
class Event {
 public:
  using Handler = std::function<void()>;

  Event(int event_fd, Handler handler);
  ~Event();

  // This class is neither copyable nor movable.
  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;

 private:
  // Monitors eventfd_. Runs on thread_.
  void Monitor(int event_fd, const Handler& handler);

  // Convenience function to read |enabled_| with locks held.
  bool IsEnabled() const LOCKS_EXCLUDED(mutex_);

  // Event fd.
  const int event_fd_;

  // Mutex that guards enabled_;
  mutable std::mutex mutex_;

  // Enabled if true.
  bool enabled_ GUARDED_BY(mutex_){true};

  // Thread for monitoring interrupts.
  std::thread thread_;
};

// Implements a mechanism for processing kernel events.
class KernelEventHandler {
 public:
  KernelEventHandler(const std::string& device_path, int num_events);
  ~KernelEventHandler() = default;

  util::Status Open() LOCKS_EXCLUDED(mutex_);
  util::Status Close() LOCKS_EXCLUDED(mutex_);

  // Registers and enables the specified event.
  util::Status RegisterEvent(int event_id, Event::Handler handler)
      LOCKS_EXCLUDED(mutex_);

 private:
  // Maps the specified event number with the specified id.
  util::Status SetEventFd(int event_fd, int event_id) const
      SHARED_LOCKS_REQUIRED(mutex_);

  // Device path.
  const std::string device_path_;

  // Number of events.
  const int num_events_;

  // Mutex that guards fd_, event_fd_, interrupts_;
  mutable std::mutex mutex_;

  // File descriptor of the opened device.
  int fd_ GUARDED_BY(mutex_){-1};

  // Event FD list.
  std::vector<int> event_fds_ GUARDED_BY(mutex_);

  // Registered events.
  std::vector<std::unique_ptr<Event>> events_ GUARDED_BY(mutex_);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_EVENT_HANDLER_H_
