// Copyright (C) 2014-2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#endif

#include <new>
#include <type_traits>

namespace android {
namespace base {
namespace internal {

// A LazyInstance is a helper template that can be used to perform
// thread-safe lazy initialization of static C++ objects without forcing
// the generation of C++ static constructors in the final executable.
//
// In a nutshell, you can replace a statement like:
//
//    static Foo gFoo;
//
// With:
//
//    static LazyInstance<Foo> gFoo = LAZY_INSTANCE_INIT;
//
// In the first case, a hidden static C++ constructor is embedded in the
// final executable, and executed at *load* *time* to call the Foo::Foo
// constructor on the gFoo object.
//
// On the second case, gFoo will only be initialized lazily, i.e. the first
// time any code actually tries to access the variable.
//
// Note that access is slightly different, i.e.:
//
//    gFoo.get() returns a reference to the lazy-initialized object.
//    gFoo.ptr() returns a pointer to it.
//    gFoo->Something() is equivalent to doing gFoo.ptr()->Something().
//
// 'gFoo' is stored in the .bss section and this doesn't use heap allocation.
// This class can only be used to perform lazy initialization through the
// class' default constructor. For more specialized cases, you will have
// to create a derived class, e.g.:
//
//    class FoorWithDefaultParams : public Foo {
//    public:
//       FooWithDefaultParams() : Foo(<default-parameters>) {}
//    };
//
//    LazyInstance<FooWithDefaultParams> gFoo = LAZY_INSTANCE_INIT;
//
// The implementation of LazyInstance relies on atomic operations and
// POD-struct class definitions, i.e. one that doesn't have any constructor,
// destructor, virtual members, or private ones, and that can be
// zero-initialized at link time.
//
// You can also use LazyInstance<> instances as static local variables,
// e.g.:
//
//     Foo*  getFooSingleton() {
//        static LazyInstance<Foo> sFoo = LAZY_INSTANCE_INIT;
//        return sFoo.ptr();
//     }
//
// This is useful on Windows which doesn't support thread-safe lazy
// initialization of static C++ local variables, or on other platforms
// when the code is compiled with -fno-threadsafe-statics.
//
// This class is heavily inspired by Chromium's implementation of the
// same-named class (see $CHROMIUM/src/base/lazy_instance.h).

// Atomic state variable type. Used to ensure to synchronize concurrent
// initialization and access without incurring the full cost of a mutex
// lock/unlock.
struct LazyInstanceState {
    enum {
        STATE_INIT = 0,
        STATE_CONSTRUCTING = 1,
        STATE_DONE = 2,
    };

    bool inInitState();
    bool needConstruction();
    void doneConstructing();

#ifdef _WIN32
    typedef LONG volatile AtomicType;
#else
    typedef int volatile AtomicType;
#endif

    volatile AtomicType mState;
};

}  // namespace internal

// LazyInstance template definition, see comment above for usage
// instructions. It is crucial to make this a POD-struct compatible
// type [1].
//
// [1] http://en.wikipedia.org/wiki/Plain_Old_Data_Structures
//
template <class T>
struct LazyInstance {
    bool hasInstance() const { return !mState.inInitState(); }

    const T& get() const { return *ptr(); }
    T& get() { return *ptr(); }

    const T* operator->() const { return ptr(); }
    T* operator->() { return ptr(); }

    const T& operator*() const { return get(); }
    T& operator*() { return get(); }

    T* ptr() { return ptrInternal(); }
    const T* ptr() const { return ptrInternal(); }

private:
    T* ptrInternal() const;

    using StorageT =
            typename std::aligned_storage<
                    sizeof(T),
                    std::alignment_of<T>::value
            >::type;

    alignas(double) mutable internal::LazyInstanceState mState;
    mutable StorageT mStorage;
};

// Initialization value, must resolve to all-0 to ensure the object
// instance is actually placed in the .bss
#define LAZY_INSTANCE_INIT  { }

template <class T>
T* LazyInstance<T>::ptrInternal() const {
    // make sure that LazyInstance<> instantiation remains POD
    // NB: this can't go in a class scope - one needs a way of specifying
    // current instantiation type without explicitly saying LazyInstance<T>
    // which is a recursive instantiation and gives compiler error
    // decltype(*this) is a way of doing it, but it only works in a function
    static_assert(std::is_pod<
                          typename std::decay<decltype(*this)>::type
                  >::value,
                  "LazyInstance<T> is not a POD type");

    if (mState.needConstruction()) {
        new (&mStorage) T();
        mState.doneConstructing();
    }
    return reinterpret_cast<T*>(&mStorage);
}

}  // namespace base
}  // namespace android
