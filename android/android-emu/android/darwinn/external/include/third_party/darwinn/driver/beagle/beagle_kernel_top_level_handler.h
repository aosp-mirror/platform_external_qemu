#ifndef THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_KERNEL_TOP_LEVEL_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_KERNEL_TOP_LEVEL_HANDLER_H_

#include <mutex>  // NOLINT

#include "third_party/darwinn/api/driver_options_generated.h"
#include "third_party/darwinn/driver/top_level_handler.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Handles chip specific resets.
class BeagleKernelTopLevelHandler : public TopLevelHandler {
 public:
  BeagleKernelTopLevelHandler(const std::string &device_path,
                              api::PerformanceExpectation performance);
  ~BeagleKernelTopLevelHandler() override = default;

  // Implements ResetHandler interface.
  util::Status Open() override;
  util::Status Close() override;
  util::Status EnableSoftwareClockGate() override;
  util::Status DisableSoftwareClockGate() override;
  util::Status QuitReset() override;

 private:
  // Device path.
  const std::string device_path_;

  // File descriptor of the opened device.
  int fd_ GUARDED_BY(mutex_){-1};

  // Mutex that guards fd_.
  std::mutex mutex_;

  // Chip starts in clock gated state.
  bool clock_gated_{true};

  // Performance setting.
  const api::PerformanceExpectation performance_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_KERNEL_TOP_LEVEL_HANDLER_H_
