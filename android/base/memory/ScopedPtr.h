// Copyright 2014 The Android Open Source Project
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

#include "android/base/Compiler.h"

#include <stddef.h>
#include <stdlib.h>

namespace android {
namespace base {

// Helper scoped pointer template class. Ensures that an object is
// deleted at scope exit, unless its release() method has been called.
// Usage example:
//
//    {
//      ScopedPtr<Foo> ptr(new Foo());
//      foo->DoStuff();
//    }  // destroys the instance automatically.
//
// You cannot copy ScopedPtr instances directly, but you can actually
// pass ownership from one instance to the other with the release()
// method as in:
//
//     ScopedPtr<Foo> ptr(new Foo());         // object owned by |ptr|
//     ScopedPtr<Foo> ptr2(ptr.release());    // object owned by |ptr2|
//
// ScopedPtr also supports custom deleter classes. The default one just
// calls 'delete ptr'.

struct DefaultDelete {
    template <class T>
    void operator()(T ptr) const {
        delete ptr;
    }
};

struct FreeDelete {
    template <class T>
    void operator()(T ptr) const {
        free((void*)ptr);
    }
};

template <class T, class Deleter = DefaultDelete>
class ScopedPtr : private Deleter {
public:
    // Default constructor.
    ScopedPtr() : mPtr(nullptr) {}

    // Normal constructor, takes ownership of |ptr|.
    explicit ScopedPtr(T* ptr) : mPtr(ptr) {}

    // Destructor will call reset() automatically.
    ~ScopedPtr() { reset(nullptr); }

    // Release the pointer object from the instance and return it.
    // Caller should assume ownership of the object.
    T* release() {
        T* ptr = mPtr;
        mPtr = nullptr;
        return ptr;
    }

    // Reset the managed object to a new value.
    void reset(T* ptr) {
        static_cast<Deleter&>(*this)(mPtr);
        mPtr = ptr;
    }

    // Check if pointer is not null
    explicit operator bool() const {
        return mPtr != nullptr;
    }

    // Return pointer of scoped object. Owernship is _not_
    // transferred to the caller.
    T* get() const { return mPtr; }

    // Return a reference to the scoped object. Allows one to
    // write (*foo).DoStuff().
    T& operator*() const { return *mPtr; }

    // Return a pointer to the scoped object. Allows one to write
    // foo->DoStuff().
    T* operator->() const { return mPtr; }

private:
    DISALLOW_COPY_AND_ASSIGN(ScopedPtr);
    T* mPtr;
};

template <class T>
using ScopedCPtr = ScopedPtr<T, FreeDelete>;

}  // namespace base
}  // namespace android
