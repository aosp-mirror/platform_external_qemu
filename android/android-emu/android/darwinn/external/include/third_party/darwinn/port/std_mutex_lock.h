#ifndef THIRD_PARTY_DARWINN_PORT_STD_MUTEX_LOCK_H_
#define THIRD_PARTY_DARWINN_PORT_STD_MUTEX_LOCK_H_

#include <mutex>  // NOLINT

#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {

// A wrapper around std::mutex lockers to enable thread annotations. The
// constructor takes a pointer to a mutex, which resembles MutexLock interface.
template<typename T>
class SCOPED_LOCKABLE AnnotatedStdMutexLock : public T {
 public:
  explicit AnnotatedStdMutexLock(std::mutex* mu) EXCLUSIVE_LOCK_FUNCTION(mu)
      : T(*mu) {}

  // This class is neither copyable nor movable.
  AnnotatedStdMutexLock(const AnnotatedStdMutexLock&) = delete;
  AnnotatedStdMutexLock& operator=(const AnnotatedStdMutexLock&) = delete;

  ~AnnotatedStdMutexLock() UNLOCK_FUNCTION() = default;
};

// Intended to be used as a direct replacement of ReaderMutexLock/MutexLock. The
// mutex is locked when constructed, and unlocked when destructed.
typedef AnnotatedStdMutexLock<std::lock_guard<std::mutex>> StdMutexLock;

// Intended to be used as a direct replacement of ReaderMutexLock/MutexLock only
// when std::condition_variable is used with the mutex. Use StdMutexLock
// otherwise. The mutex is locked when constructed, and unlocked when
// destructed.
typedef AnnotatedStdMutexLock<std::unique_lock<std::mutex>> StdCondMutexLock;

}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_PORT_STD_MUTEX_LOCK_H_
