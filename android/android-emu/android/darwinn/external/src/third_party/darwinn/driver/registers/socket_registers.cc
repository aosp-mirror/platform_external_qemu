#include "third_party/darwinn/driver/registers/socket_registers.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

#include "third_party/darwinn/port/cleanup.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

SocketRegisters::SocketRegisters(const string& ip_address, int port)
    : ip_address_(ip_address), port_(port) {}

SocketRegisters::~SocketRegisters() {
  if (socket_fd_ != -1) {
    LOG(WARNING)
        << "Destroying SocketRegisters - Close() has not yet been called!";
    util::Status status = Close();
    if (!status.ok()) {
      LOG(ERROR) << status;
    }
  }
}

util::Status SocketRegisters::Open() {
  StdMutexLock lock(&mutex_);
  if (socket_fd_ != -1) {
    return util::FailedPreconditionError("Socket already open.");
  }

  VLOG(1) << StringPrintf("Opening socket at %s:%d", ip_address_.c_str(),
                          port_);

  if ((socket_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return util::UnavailableError(StringPrintf("socket failed (%d).", errno));
  }

  // Clean up on error.
  auto socket_closer = gtl::MakeCleanup(
      [this]() EXCLUSIVE_LOCKS_REQUIRED(mutex_) { close(socket_fd_); });

  // Setup server address.
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_);

  if (inet_pton(AF_INET, ip_address_.c_str(), &server_addr.sin_addr) <= 0) {
    return util::FailedPreconditionError(
        StringPrintf("Invalid ip address: %s", ip_address_.c_str()));
  }

  // Make connection.
  if (connect(socket_fd_, reinterpret_cast<sockaddr*>(&server_addr),
              sizeof(server_addr)) < 0) {
    return util::UnavailableError(StringPrintf("connect failed (%d).", errno));
  }

  socket_closer.release();
  return util::Status();  // OK
}

util::Status SocketRegisters::Close() {
  StdMutexLock lock(&mutex_);
  if (socket_fd_ == -1) {
    return util::FailedPreconditionError("Socket already closed.");
  }

  close(socket_fd_);
  return util::Status();  // OK
}

util::Status SocketRegisters::Write(uint64 offset, uint64 value) {
  VLOG(2) << StringPrintf(
      "Register write 0x%llx to 0x%llx",
      static_cast<unsigned long long>(value),    // NOLINT(runtime/int)
      static_cast<unsigned long long>(offset));  // NOLINT(runtime/int)
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(Send('w'));
  RETURN_IF_ERROR(Send(offset));
  RETURN_IF_ERROR(Send(value));
  return util::Status();  // OK
}

util::StatusOr<uint64> SocketRegisters::Read(uint64 offset) {
  VLOG(2) << StringPrintf(
      "Register read from 0x%llx",
      static_cast<unsigned long long>(offset));  // NOLINT(runtime/int)
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(Send('r'));
  RETURN_IF_ERROR(Send(offset));
  uint64 value;
  if (recv(socket_fd_, &value, sizeof(value), MSG_WAITALL) < 0) {
    return util::UnavailableError(StringPrintf("recv failed (%d).", errno));
  }
  return value;
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
