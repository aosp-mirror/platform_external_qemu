#ifndef THIRD_PARTY_DARWINN_API_DRIVER_OPTIONS_HELPER_H_
#define THIRD_PARTY_DARWINN_API_DRIVER_OPTIONS_HELPER_H_

#include "third_party/darwinn/api/driver.h"

namespace platforms {
namespace darwinn {
namespace api {

// A simpler wrapper around several static helper functions.
class DriverOptionsHelper {
 public:
  // Returns driver options for default.
  static Driver::Options Defaults();

  // Returns driver options for maximum performance.
  static Driver::Options MaxPerformance();
};

}  // namespace api
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_DRIVER_OPTIONS_HELPER_H_
