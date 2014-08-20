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

#ifndef ANDROID_BASE_CONTAINERS_POINTER_SET_H
#define ANDROID_BASE_CONTAINERS_POINTER_SET_H

#include "android/base/Compiler.h"
#include "android/base/containers/HashUtils.h"

namespace android {
namespace base {

// A PointerSet<T> models a set referencing heap-allocated objects of type T.
//
// The objects are not owned by the PointerSet<T>, see ScopedPointerSet<T>
// instead if you want the set to also own the objects.
//
// Each object is identified by its address which serves as the hashing
// key. This makes the implementation simple, but also ensures that
// iterating over the same-constructed set will not provide the same
// result between two runs of the same program.
//
// Usage example:
//
//     PointerSet<Foo> setOfFoos;   // Creates new empty set.
//     setOfFoos.add(foo1);         // Add foo1 to the set.
//     if (setOfFoos.contains(foo1)) {  // Test whether the set contains foo1
//        setOfFoos.remove(foo1);   // Remove foo1 from set.
//     }
//     CHECK(setOfFoos.empty());
//     CHECK(setOfFoos.size() == 0);
//
//  Iterating over a set:
//
//     PointerSet<Foo>::Iterator  iter(&setOfFoos);
//     while (iter.hasNext()) {
//        Foo* foo = iter.next();
//        .. do something with foo
//     }
//
//  Note that the iterator becomes invalid if you add or remove objects
//  to/from the set.

class PointerSetBase {
public:
    PointerSetBase();
    ~PointerSetBase();

    class Iterator {
    public:
        explicit Iterator(PointerSetBase* set);
        ~Iterator() {}

        inline bool hasNext() const {
            return (mPos < mCapacity);
        }

        void* next();
    private:
        // No default constructor
        Iterator();

        DISALLOW_COPY_AND_ASSIGN(Iterator);

        void** mItems;
        size_t mCapacity;
        size_t mPos;
    };

    typedef size_t (*HashFunction)(const void* obj);

    struct DefaultTraits {
        static size_t hash(const void* obj) {
            return ::android::base::internal::pointerHash(obj);
        }
    };

protected:
    bool empty() const {
        return mCount == 0;
    }

    size_t size() const {
        return mCount;
    }

    bool contains(const void* obj, HashFunction hashFunc) const;

    void clear();

    void* addItem(void* obj, HashFunction hashFunc);

    void* removeItem(void* obj, HashFunction hashFunc);

    void** toArray() const;

private:
    //friend class PointerSetBase::Iterator;

    void maybeResize(HashFunction hashFunc);

    size_t mShift;
    size_t mCount;
    void** mItems;
};

template <typename T,
          typename TRAITS = typename ::android::base::PointerSetBase::DefaultTraits>
class PointerSet : public PointerSetBase {
public:
    // Default constructor creates an empty set.
    PointerSet() : PointerSetBase() {};

    // Destructor simply destroys the set, not the objects in it.
    ~PointerSet() {}

    // Return true iff the set is empty.
    bool empty() const {
        return PointerSetBase::empty();
    }

    // Return the number of objects in the set.
    size_t size() const {
        return PointerSetBase::size();
    }

    // Return true iff the set contains object |obj|. Always return false
    // if |obj| is NULL.
    bool contains(const T* obj) const {
        return PointerSetBase::contains(static_cast<const void*>(obj),
                                        TRAITS::hash);
    }

    // Remove all objects from the set.
    void clear() {
        PointerSetBase::clear();
    }

    // Add object |obj| to the set. Return true if the object was added,
    // and false if it was already in the set. Always return false if
    // |obj| is NULL.
    bool add(T* obj) {
        return PointerSetBase::addItem(obj, TRAITS::hash) != NULL;
    }

    // Remove object |obj| from the set. Return true if the object was
    // already in the set, and false otherwise. Return false if |obj|
    // is NULL.
    bool remove(T* obj) {
        return PointerSetBase::removeItem(obj, TRAITS::hash) != NULL;
    }

    // Iterator class for this set. Note that adding or removing items
    // in the set makes the iterator invalid. Usage example is:
    //
    //     PointerSet<Foo>::Iterator  iter(&fooSet);
    //     while (iter.hasNext()) {
    //        Foo* foo = iter.next();
    //        .. do something with |foo|
    //     }
    class Iterator : public PointerSetBase::Iterator {
    public:
        Iterator(PointerSet* set) : PointerSetBase::Iterator(set) {}

        bool hasNext() const {
            return PointerSetBase::Iterator::hasNext();
        }

        T* next() {
            return reinterpret_cast<T*>(PointerSetBase::Iterator::next());
        }
    };

    // Create a heap-allocated array of size() pointers to the objects
    // contained in the set. The array must be free()-ed by the caller.
    // Return NULL if the set is empty.
    T** toArray() const {
        return reinterpret_cast<T**>(PointerSetBase::toArray());
    }
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_CONTAINERS_POINTER_SET_H
