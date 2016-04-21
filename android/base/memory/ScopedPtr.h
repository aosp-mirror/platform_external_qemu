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

#include <memory>
#include <type_traits>

#include <stdlib.h>

namespace android {
namespace base {

struct FreeDelete {
    template <class T>
    void operator()(T ptr) const {
        free((void*)ptr);
    }
};

template <class Func>
struct FuncDelete {
    explicit FuncDelete(Func f) : mF(f) {}

    template <class T>
    void operator()(T* t) const {
        if (mF) {
            mF(const_cast<typename std::remove_const<T>::type*>(t));
        }
    }

    // Allow a user to update the deleter on the fly, e.g. if one wants to
    // replace a heap-allocated buffer with a stack-allocated one without having
    // complicated conditions in the code.
    void reset(Func f) {
        mF = f;
    }

private:
    Func mF;
};

template <class T, class Deleter = std::default_delete<T>>
using ScopedPtr = std::unique_ptr<T, Deleter>;

template <class T>
using ScopedCPtr = std::unique_ptr<T, FreeDelete>;

template <class T, class Func>
using ScopedCustomPtr = std::unique_ptr<T, FuncDelete<Func>>;

// A factory function that creates a scoped pointer with |deleter|
// function used as a deleter - it is called at the scope exit.
template <class T, class Func>
ScopedCustomPtr<typename std::remove_extent<T>::type, typename std::decay<Func>::type>
makeCustomScopedPtr(T* data, Func deleter) {
    return ScopedCustomPtr<typename std::remove_extent<T>::type,
                           typename std::decay<Func>::type>(
            data, FuncDelete<typename std::decay<Func>::type>(deleter));
}

}  // namespace base
}  // namespace android
