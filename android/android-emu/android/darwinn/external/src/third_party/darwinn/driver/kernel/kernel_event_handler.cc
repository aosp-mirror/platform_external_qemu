#include "third_party/darwinn/driver/kernel/kernel_event_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>
#include <thread>  // NOLINT
#include <utility>

#include "third_party/darwinn/driver/kernel/linux_gasket_ioctl.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

Event::Event(int event_fd, Handler handler) : event_fd_(event_fd) {
  std::thread event_thread(&Event::Monitor, this, event_fd, std::move(handler));
  thread_ = std::move(event_thread);
}

Event::~Event() {
  // Mark as disabled.
  {
    StdMutexLock lock(&mutex_);
    enabled_ = false;
  }

  // Write a fake event to force read() to return.
  uint64 num_events = 1;
  int result = write(event_fd_, &num_events, sizeof(num_events));
  if (result != sizeof(num_events)) {
    LOG(WARNING) << StringPrintf("event_fd=%d. Fake event write failed (%d).",
                                 event_fd_, result);
  }

  // Wait for thread to exit.
  thread_.join();
}

bool Event::IsEnabled() const {
  StdMutexLock lock(&mutex_);
  return enabled_;
}

void Event::Monitor(int event_fd, const Handler& handler) {
  VLOG(5) << StringPrintf("event_fd=%d. Monitor thread begin.", event_fd);

  while (IsEnabled()) {
    // Wait for events (blocking.).
    uint64 num_events = 0;
    int result = read(event_fd, &num_events, sizeof(num_events));
    if (result != sizeof(num_events)) {
      LOG(WARNING) << StringPrintf("event_fd=%d. Read failed (%d).", event_fd,
                                   result);
      break;
    }

    VLOG(5) << StringPrintf("event_fd=%d. Monitor thread got num_events=%lld.",
                            event_fd, num_events);
    if (IsEnabled()) {
      for (int i = 0; i < num_events; ++i) {
        handler();
      }
    }
  }

  VLOG(5) << StringPrintf("event_fd=%d. Monitor thread exit.", event_fd);
}

KernelEventHandler::KernelEventHandler(const std::string& device_path,
                                       int num_events)
    : device_path_(device_path), num_events_(num_events) {
  event_fds_.resize(num_events_, -1);
  events_.resize(num_events_);
}

util::Status KernelEventHandler::Open() {
  StdMutexLock lock(&mutex_);
  if (fd_ != -1) {
    return util::FailedPreconditionError("Device already open.");
  }

  fd_ = open(device_path_.c_str(), O_RDWR);
  if (fd_ < 0) {
    return util::FailedPreconditionError(
        StringPrintf("Device open failed : %d (%s)", fd_, strerror(errno)));
  }

  for (int i = 0; i < num_events_; ++i) {
    event_fds_[i] = eventfd(0, EFD_CLOEXEC);
    events_[i].reset();
  }

  return util::Status();  // OK
}

util::Status KernelEventHandler::Close() {
  StdMutexLock lock(&mutex_);
  if (fd_ == -1) {
    return util::FailedPreconditionError("Device not open.");
  }

  for (int i = 0; i < num_events_; ++i) {
    events_[i].reset();
    close(event_fds_[i]);
  }

  close(fd_);
  fd_ = -1;

  return util::Status();  // OK
}

util::Status KernelEventHandler::SetEventFd(int event_fd, int event_id) const {
  gasket_interrupt_eventfd interrupt;
  interrupt.interrupt = event_id;
  interrupt.event_fd = event_fd;
  if (ioctl(fd_, GASKET_IOCTL_SET_EVENTFD, &interrupt) != 0) {
    return util::FailedPreconditionError(StringPrintf(
        "Setting Event Fd Failed : %d (%s)", fd_, strerror(errno)));
  }

  VLOG(5) << StringPrintf("Set event fd : event_id:%d -> event_fd:%d, ",
                          event_id, event_fd);

  return util::Status();  // OK
}

util::Status KernelEventHandler::RegisterEvent(int event_id,
                                               Event::Handler handler) {
  StdMutexLock lock(&mutex_);
  if (fd_ == -1) {
    return util::FailedPreconditionError("Device not open.");
  }

  RETURN_IF_ERROR(SetEventFd(event_fds_[event_id], event_id));

  // Enable events.
  events_[event_id] =
      gtl::MakeUnique<Event>(event_fds_[event_id], std::move(handler));

  return util::Status();  // OK;
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
