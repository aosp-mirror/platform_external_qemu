#ifndef THIRD_PARTY_DARWINN_DRIVER_REGISTERS_SOCKET_REGISTERS_H_
#define THIRD_PARTY_DARWINN_DRIVER_REGISTERS_SOCKET_REGISTERS_H_

#include <errno.h>
#include <sys/socket.h>
#include <mutex>  // NOLINT
#include <string>

#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/stringprintf.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Socket implementation of the register interface that sends requests through
// socket and receives the results back through socket.
//
// Commands are sent as following:
// 1. 'r' or 'w' depending on read/write.
// 2. Offset for both read/write.
// 3. If write, value to write.
class SocketRegisters : public Registers {
 public:
  SocketRegisters(const string& ip_address, int port);
  ~SocketRegisters() override;

  // This class is neither copyable nor movable.
  SocketRegisters(const SocketRegisters&) = delete;
  SocketRegisters& operator=(const SocketRegisters&) = delete;

  // Overrides from registers.h
  util::Status Open() LOCKS_EXCLUDED(mutex_) override;
  util::Status Close() LOCKS_EXCLUDED(mutex_) override;
  util::Status Write(uint64 offset, uint64 value)
      LOCKS_EXCLUDED(mutex_) override;
  util::StatusOr<uint64> Read(uint64 offset) LOCKS_EXCLUDED(mutex_) override;
  util::Status Write32(uint64 offset, uint32 value)
      LOCKS_EXCLUDED(mutex_) override {
    return Write(offset, value);
  }
  util::StatusOr<uint32> Read32(uint64 offset) LOCKS_EXCLUDED(mutex_) override {
    return Read(offset);
  }

 private:
  template <typename T>
  util::Status Send(const T& message) EXCLUSIVE_LOCKS_REQUIRED(mutex_) {
    if (send(socket_fd_, &message, sizeof(message), /*flags=*/0) < 0) {
      return util::UnavailableError(StringPrintf("send failed (%d).", errno));
    }
    return util::Status();  // OK
  }

  // IP address.
  const string ip_address_;

  // Port number.
  const int port_;

  // Mutex that guards socket_fd_;
  std::mutex mutex_;

  // Socket descriptor.
  int socket_fd_ GUARDED_BY(mutex_){-1};
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_REGISTERS_SOCKET_REGISTERS_H_
