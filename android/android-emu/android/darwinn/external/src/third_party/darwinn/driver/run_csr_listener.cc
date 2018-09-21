// A tool that listens for CSR read/write requests on a socket, does CSR access
// on hardware, and writes results back to the socket if request was read.
// Requests are pipelined.
//
// Example usage for sending a file with CSR read/writes with nc.
//  - cat /tmp/csrs.txt | nc fden39.svl.corp.google.com 9999

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>

#include "third_party/darwinn/driver/kernel/kernel_registers.h"
#include "third_party/darwinn/driver/memory/address_utilities.h"
#include "third_party/darwinn/port/cleanup.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/gflags.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/stringprintf.h"

DEFINE_string(mmap_base_address, "", "mmap() base address.");
DEFINE_string(mmap_size_bytes, "", "mmap() size bytes.");

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

// Default port to bind to.
constexpr int kDefaultPort = 9999;

// Reads host physical memory with /dev/mem.
util::Status HostMemoryRead(uint64 address, uint64* value) {
  int fd = open("/dev/mem", O_RDONLY);
  if (fd < 0) {
    return util::FailedPreconditionError("/dev/mem open failed.");
  }

  uint64 address_base = GetPageAddress(address);
  uint64 page_offset = GetPageOffset(address);

  uint64* mmap_ptr =
      static_cast<uint64*>(mmap(nullptr, sizeof(uint64), PROT_READ,
                                MAP_SHARED | MAP_LOCKED, fd, address_base));
  if (mmap_ptr == MAP_FAILED) {
    close(fd);
    return util::FailedPreconditionError(
        StringPrintf("Could not mmap: %s.", strerror(errno)));
  }

  *value = mmap_ptr[page_offset / sizeof(*mmap_ptr)];

  // Cleanup.
  munmap(mmap_ptr, sizeof(uint64));
  close(fd);

  return util::Status();  // OK.
}

}  // namespace

class CsrListener {
 public:
  CsrListener(uint64 mmap_base_address, uint64 mmap_size_bytes) {
    registers_ = gtl::MakeUnique<KernelRegisters>(
        "/dev/mem", mmap_base_address, mmap_size_bytes, /*read_only=*/false);
    CHECK_OK(registers_->Open());
  }

  ~CsrListener() { CHECK_OK(registers_->Close()); }

  // Listens and services connections. Does not return unless there is an error.
  util::Status StartServer() {
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(kDefaultPort);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      return util::UnavailableError(StringPrintf("socket failed (%d).", errno));
    }

    // Clean up on error.
    auto sockfd_closer = gtl::MakeCleanup([sockfd] { close(sockfd); });

    // Value for SO_REUSEADDR that lets us start up a server immediately after
    // it exits.
    int opt_val = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val,
                   sizeof(opt_val)) < 0) {
      return util::UnavailableError(
          StringPrintf("setsockopt failed (%d).", errno));
    }

    int result = bind(sockfd, reinterpret_cast<struct sockaddr*>(&serv_addr),
                      sizeof(serv_addr));
    if (result < 0) {
      return util::UnavailableError(StringPrintf("bind failed (%d).", errno));
    }

    result = listen(sockfd, /*backlog=*/64);
    if (result < 0) {
      return util::UnavailableError(StringPrintf("listen failed (%d).", errno));
    }

    // Accept one connection at a time.
    while (true) {
      int socket = accept(sockfd, /*addr=*/nullptr, /*addrlen=*/nullptr);
      if (socket < 0) {
        return util::UnavailableError(
            StringPrintf("accept failed (%d).", errno));
      }

      auto socket_closer = gtl::MakeCleanup([socket] { close(socket); });
      RETURN_IF_ERROR(HandleConnection(socket));
    }

    return util::Status();  // OK
  }

 private:
  // Handles a single connection to completion.
  util::Status HandleConnection(int socket) {
    while (true) {
      // Command should come in first.
      char command;
      int result = recv(socket, &command, sizeof(command), MSG_WAITALL);
      if (result < 0) {
        return util::UnavailableError(StringPrintf("recv failed (%d).", errno));
      }

      switch (command) {
        case 'w':  // Register Write.
        case 'r':  // Register Read.
        case 'h':  // Host Memory Read.
          break;

        default:
          return util::InvalidArgumentError(
              StringPrintf("Use 'r' for CSR read, 'w' for CSR write and 'h' "
                           "for reading host memory. '%c' is unknown command.",
                           command));
      }

      // Offset comes in second for both commands.
      uint64 offset;
      result = recv(socket, &offset, sizeof(offset), MSG_WAITALL);
      if (result < 0) {
        return util::UnavailableError(StringPrintf("recv failed (%d).", errno));
      }

      // Perform CSR Write.
      if (command == 'w') {
        // Write requires another message that contains value.
        uint64 value;
        result = recv(socket, &value, sizeof(value), MSG_WAITALL);
        if (result < 0) {
          return util::UnavailableError(
              StringPrintf("recv failed (%d).", errno));
        }

        RETURN_IF_ERROR(registers_->Write(offset, value));
      }

      // Perform CSR Read.
      if (command == 'r') {
        // Return value for read command.
        ASSIGN_OR_RETURN(const uint64 value, registers_->Read(offset));
        result = send(socket, &value, sizeof(value), /*flags=*/0);
        if (result < 0) {
          return util::UnavailableError(
              StringPrintf("send failed (%d).", errno));
        }
      }

      // Perform Host Memory Read.
      if (command == 'h') {
        uint64 value;
        RETURN_IF_ERROR(HostMemoryRead(offset, &value));

        result = send(socket, &value, sizeof(value), /*flags=*/0);
        if (result < 0) {
          return util::UnavailableError(
              StringPrintf("send failed (%d).", errno));
        }
      }
    }

    return util::Status();  // OK
  }

  // Kernel registers.
  std::unique_ptr<KernelRegisters> registers_;
};

int run_main(int argc, char** argv) {
  ParseFlags(argv[0], &argc, &argv, true);

  QCHECK(!FLAGS_mmap_base_address.empty())
      << "mmap() base address not specified.";
  QCHECK(!FLAGS_mmap_size_bytes.empty()) << "mmap() size not specified.";

  const uint64 mmap_base_address = strtoull(  // NOLINT: for strtoull.
      FLAGS_mmap_base_address.c_str(), nullptr,
      /*base=*/0);
  const uint64 mmap_size_bytes = strtoull(  // NOLINT: for strtoull.
      FLAGS_mmap_size_bytes.c_str(), nullptr,
      /*base=*/0);

  auto listener =
      gtl::MakeUnique<CsrListener>(mmap_base_address, mmap_size_bytes);
  CHECK_OK(listener->StartServer());

  return 0;
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

int main(int argc, char** argv) {
  return platforms::darwinn::driver::run_main(argc, argv);
}
