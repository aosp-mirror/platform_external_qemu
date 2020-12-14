// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/TypeTraits.h"

#include <utility>

namespace android {
namespace base {

//
// FunctionView<> - a class that stores a view of a callable object, similar
//      to StringView for strings.
//
// Emulator needs to pass around lots of callbacks. The standard way of doing it
// is via std::function<> class, and it usually works OK. But there are some
// noticeable drawbacks:
//
//  1. std::function<> in most STLs performs heap allocation for all callables
//     bigger than a single poiner to a function.
//  2. std::function<> goes through at least two pointers + a vptr call to call
//     the stored function.
//  3. std::function<> copies the passed object inside at least once; this also
//     means it can't work with non-copyable functors.
//
// FunctionView is an alternative way of passing functors around. Instead of
// storing a copy of the functor inside, it follows the path of StringView and
// merely captures a pointer to the object to call. This allows for a simple,
// fast and lightweight wrapper design; it also dictates the limitations:
//
//  1. FunctionView<> stores a pointer to outside functor. That functor _must_
//     outlive the view.
//  2. FunctionView<> has two calls through a function pointer in its call
//     operator. That's still better than std::function<>, but slower compared
//     to a raw function pointer call with a "void* opaque" context parameter.
//
// Limitation #1 dictates the best use case: a function parameter type for some
// generic callback which doesn't get stored inside an object field but only
// gets called in this call. E.g.:
//
//  void someLongOperation(FunctionView<void(int progress)> onProgress) {
//      firstStep(onProgress);
//      ...
//      onProgress(50);
//      ...
//      lastStep(onProgress);
//      onProgress(100);
//  }
//
// In this code std::function<> is an overkill as the whole use of |onProgresss|
// callback is scoped and easy to track. An alternative design - making it a
// template parameter (template <class Callback> ... (Callback onProgress))
// forces one to put someLongOperation() + some private functions into the
// header. FunctionView<> is the choice then.
//
// NOTE: Beware of passing temporary functions via FunctionView<>! Temporaries
//  live until the end of full expression (usually till the next semicolon), and
//  having a FunctionView<> that refers to a dangling pointer is a bug that's
//  hard to debug. E.g.:
//      FunctionView<...> v = [](){};                     // this is fine
//      FunctionView<...> v = std::function<...>([](){}); // this will kill you
//
// NOTE2: FunctionView<> has an empty state, like a normal function pointer or
//  std::function<>. Don't forget to check it before calling, or use some empty
//  function as a sentinel value. Calling empty view will crash your app.

template <class Signature>
class FunctionView;

template <class Ret, class... Args>
class FunctionView<Ret(Args...)> final {
public:
    FunctionView() : mTypeErasedFunction() {}

    template <class Callable,
              class = android::base::enable_if_c<
                      is_callable_as<Callable, Ret(Args...)>::value &&
                      !std::is_same<FunctionView,
                                    typename std::remove_reference<
                                            Callable>::type>::value> >
    FunctionView(Callable&& c)
        : mTypeErasedFunction(
                  [](const FunctionView* self, Args... args) -> Ret {
                      // Generate a lambda that remembers the type of the passed
                      // |Callable|.
                      return (*reinterpret_cast<
                              typename std::remove_reference<Callable>::type*>(
                              self->mCallable))(std::forward<Args>(args)...);
                  }),
          mCallable(reinterpret_cast<intptr_t>(&c)) {}

    Ret operator()(Args... args) const {
        return mTypeErasedFunction(this, std::forward<Args>(args)...);
    }

    operator bool() const { return mTypeErasedFunction != nullptr; }
    bool empty() const { return !bool(*this); }

private:
    using TypeErasedFunc = Ret(const FunctionView*, Args...);
    TypeErasedFunc* mTypeErasedFunction;
    intptr_t mCallable;
};

}  // namespace base
}  // namespace android
