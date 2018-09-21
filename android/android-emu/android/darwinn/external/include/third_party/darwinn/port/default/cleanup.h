// MakeCleanup(f) returns an RAII cleanup object that calls 'f' in its
// destructor. The easiest way to use MakeCleanup is with a lambda argument,
// capturing the return value in an 'auto' local variable. Most users will not
// need more sophisticated syntax than that.
//
// Example:
//   void func() {}
//     FILE* fp = fopen("data.txt", "r");
//     if (fp == nullptr) return;
//     auto fp_cleaner = gtl::MakeCleanup([fp] { fclose(fp); });
//     // No matter what, fclose(fp) will happen.
//     DataObject d;
//     while (ReadDataObject(fp, &d)) {
//       if (d.IsBad()) {
//         LOG(ERROR) << "Bad Data";
//         return;
//       }
//       PushGoodData(d);
//     }
//   }
//
// You can use Cleanup<F> directly, instead of using MakeCleanup and auto,
// but there's rarely a reason to do that.
//
// You can call 'release()' on a Cleanup object to cancel the cleanup.
//
// Style exception for rvalue references: CL/62979921.
// No NOLINT qualifiers, since cpplint's handling of && is imperfect.

#ifndef THIRD_PARTY_DARWINN_PORT_DEFAULT_CLEANUP_H_
#define THIRD_PARTY_DARWINN_PORT_DEFAULT_CLEANUP_H_

#include <type_traits>
#include <utility>

#include "third_party/darwinn/port/default/macros.h"

namespace platforms {
namespace darwinn {
namespace gtl {

// A move-only RAII object that calls a stored cleanup functor when destroyed.
// Cleanup<F> is the return type of gtl::MakeCleanup(F).
template <typename F>
class Cleanup {
 public:
  Cleanup()
      : released_(true), f_() {}

  template <typename G>
  explicit Cleanup(G&& f)  // NOLINT
      : f_(std::forward<G>(f)) {}  // NOLINT(build/c++11)

  Cleanup(Cleanup&& src)  // NOLINT
      : released_(src.is_released()), f_(src.release()) { }

  // Implicitly move-constructible from any compatible Cleanup<G>.
  // The source will be released as if src.release() were called.
  // A moved-from Cleanup can be safely destroyed or reassigned.
  template <typename G>
  Cleanup(Cleanup<G>&& src)  // NOLINT
      : released_(src.is_released()), f_(src.release()) { }

  // Assignment to a Cleanup object behaves like destroying it and making a new
  // one in its place, analogous to unique_ptr semantics.
  Cleanup& operator=(Cleanup&& src) {  // NOLINT
    if (!released_) f_();
    released_ = src.released_;
    f_ = src.release();
    return *this;
  }

  ~Cleanup() {
    if (!released_) f_();
  }

  // Releases the cleanup function instead of running it.
  // Hint: use c.release()() to run early.
  F release() {
    released_ = true;
    return std::move(f_);
  }

  bool is_released() const { return released_; }

 private:
  static_assert(!std::is_reference<F>(), "F must not be a reference");

  bool released_ = false;
  F f_;
};

template <int&... ExplicitParameterBarrier, typename F,
          typename DecayF = typename std::decay<F>::type>
ABSL_MUST_USE_RESULT Cleanup<DecayF> MakeCleanup(F&& f) {
  static_assert(sizeof...(ExplicitParameterBarrier) == 0,
                "No explicit template arguments.");
  return Cleanup<DecayF>(std::forward<F>(f));
}

}  // namespace gtl
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_PORT_DEFAULT_CLEANUP_H_
