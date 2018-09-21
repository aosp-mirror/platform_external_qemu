#include "third_party/darwinn/port/blocking_counter.h"

#include <condition_variable>  // NOLINT
#include <mutex>               // NOLINT

#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {

bool BlockingCounter::DecrementCount() {
  StdMutexLock lock(&mutex_);
  count_--;
  if (count_ < 0) {
    LOG(FATAL) << "BlockingCounter::DecrementCount() called too many times.";
  }

  if (count_ == 0) {
    cv_.notify_all();
    return true;
  }
  return false;
}

void BlockingCounter::Wait() {
  StdCondMutexLock lock(&mutex_);
  cv_.wait(lock, [this] { return count_ == 0; });
}

}  // namespace darwinn
}  // namespace platforms
