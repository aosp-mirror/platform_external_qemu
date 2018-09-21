#ifndef THIRD_PARTY_DARWINN_PORT_STRCAT_H_
#define THIRD_PARTY_DARWINN_PORT_STRCAT_H_

#include <sstream>
#include <string>

namespace platforms {
namespace darwinn {

template <typename... Args>
std::string StrCat(Args const&... args) {
  std::ostringstream stream;
  int temp[]{0, ((void)(stream << args), 0)...};
  (void)temp;
  return stream.str();
}

}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_PORT_STRCAT_H_
