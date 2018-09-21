#ifndef PLATFORMS_DARWINN_MODEL_MINMAX_H_
#define PLATFORMS_DARWINN_MODEL_MINMAX_H_

namespace platforms {
namespace darwinn {
namespace model {

// A range: a minimum and corresponding maximum.
template <typename T>
struct MinMax {
  T min;
  T max;
};

}  // namespace model
}  // namespace darwinn
}  // namespace platforms

#endif  // PLATFORMS_DARWINN_MODEL_MINMAX_H_
