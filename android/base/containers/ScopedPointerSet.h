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

#ifndef ANDROID_BASE_CONTAINERS_SCOPED_POINTER_SET_H
#define ANDROID_BASE_CONTAINERS_SCOPED_POINTER_SET_H

#include "android/base/Compiler.h"
#include "android/base/containers/PointerSet.h"

namespace android {
namespace base {

// A ScopedPointerSet,T> is a variant of PointerSet<T> that also owns
// the objects referenced by the set. In practice this means that:
//
//   - The add() method transfers ownership of the object to the set.
//
//   - The remove() method actually destroys the object removed from
//     the set.
//
//   - The clear() method destroys all objects from the set.
//
//   - The pick() method is provided to remove an object from the set
//     and return it to the caller.
//
//   - Destroying the set also destroys all objects within it.
//
template <typename T, typename TRAITS = PointerSetBase::DefaultTraits>
class ScopedPointerSet : public PointerSet<T> {
public:
    ScopedPointerSet() : PointerSet<T>() {}

    ~ScopedPointerSet() {
        clear();
    }

    typedef typename PointerSet<T>::Iterator Iterator;

    bool remove(T* obj) {
        void* old = PointerSetBase::removeItem(obj, TRAITS::hash);
        if (!old) {
            return false;
        }
        delete reinterpret_cast<T*>(old);
        return true;
    }

    T* pick(T* obj) {
        void* old = PointerSetBase::removeItem(obj, TRAITS::hash);
        return reinterpret_cast<T*>(old);
    }

    void clear() {
        Iterator iter(this);
        while (iter.hasNext()) {
            delete iter.next();
        }
        PointerSetBase::clear();
    }
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_CONTAINERS_SCOPED_POINTER_SET_H
