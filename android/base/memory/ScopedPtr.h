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

#ifndef ANDROID_BASE_MEMORY_SCOPED_PTR_H
#define ANDROID_BASE_MEMORY_SCOPED_PTR_H

#include "android/base/Compiler.h"

#include <stddef.h>

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
template <class T>
class ScopedPtr {
public:
    // Default constructor.
    ScopedPtr() : mPtr(NULL) {}

    // Normal constructor, takes ownership of |ptr|.
    explicit ScopedPtr(T* ptr) : mPtr(ptr) {}

    // Destructor will call reset() automatically.
    ~ScopedPtr() { reset(NULL); }

    // Release the pointer object from the instance and return it.
    // Caller should assume ownership of the object.
    T* release() {
        T* ptr = mPtr;
        mPtr = NULL;
        return ptr;
    }

    // Reset the managed object to a new value.
    void reset(T* ptr) {
        delete mPtr;
        mPtr = ptr;
    }

    // Return pointer of scoped object. Owernship is _not_
    // transferred to the caller.
    T* get() { return mPtr; }

    // Return a reference to the scoped object. Allows one to
    // write (*foo).DoStuff().
    T& operator*() { return *mPtr; }

    // Return a pointer to the scoped object. Allows one to write
    // foo->DoStuff().
    T* operator->() { return mPtr; }

private:
    DISALLOW_COPY_AND_ASSIGN(ScopedPtr);
    T* mPtr;
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_MEMORY_SCOPED_PTR_H
