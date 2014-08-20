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

#ifndef ANDROID_BASE_CONTAINERS_TAIL_QUEUE_LIST_H
#define ANDROID_BASE_CONTAINERS_TAIL_QUEUE_LIST_H

#include "android/base/Compiler.h"
#include "android/base/Log.h"

#include <stddef.h>

namespace android {
namespace base {

// Convenience classes and macros to define generic tail-queue lists.
//
// Generally speaking, these lists perform poorly on modern CPUs since their
// use can introduce cache trashing at runtime. However, they're still
// efficient if one wants to quickly and often remove liberal items from
// the list in an O(1) way.
//
// Usage example:
//
//   1/ The type of each item in the list must have a TailQueueLink<T> field,
//      called the 'link',  which is used by the implementation to store
//      opaque queue-related fields. For example:
//
//      struct Foo {
//          Foo() { ... }
//          TailQueueLink<Foo> mLink;
//      }
//
//   2/ The type must also have a public Traits structure that provides
//      a single function named link() used to return the address of a
//      link. One way to define it is to use the macro:
//
//          TAIL_QUEUE_LIST_TRAITS(Traits, Foo, mLink);
//
//      which defines the |Traits| structure that returns the address of
//      Foo::mLink inside a Foo instance. If you don't want to use the
//      macro, the definition must be:
//
//          struct Traits {
//              static inline TailQueueLink<Foo>* link(Foo* foo) {
//                  return &foo->mLink;
//              }
//          };
//
//   3/ Define the tail-queue-list type with:
//
//          typedef TailQueueList<Foo> FooList;
//
//      By default, this relies on Foo::Traits being defined, it is possible
//      to give the name of a different Traits structure as a second template
//      parameter. This is useful if a single object can actually be
//      contained inside two parallel lists, as in:
//
//          class Bar {
//          public:
//              Bar() { ... }
//
//              TAIL_QUEUE_LIST_TRAITS(Traits1, Bar, mLink1);
//              TAIL_QUEUE_LIST_TRAITS(Traits2, Bar, mLink2);
//          private:
//              TailQueueLink<Bar> mLink1;
//              TailQueueLink<Bar> mLink2;
//              ...
//          };
//
//          typedef TailQueueList<Bar, Traits1>  BarList1;
//          typedef TailQueueList<Bar, Traits2>  BarList2;
//
//   4/ One can use the link's next() method to get a pointer to the next
//      item in the list, or NULL if this is the last item. For example, the
//      following can be used to remove all items from a queue if |mLink| is
//      a public field.
//
//          FooList* queue = ...;
//          Foo* foo = queue->front();
//          while (foo) {
//              Foo* next = foo->mLink.next();
//              queue->remove(foo);
//              foo = next;
//          }
//
//      If the field is private, consider adding an appropriate public
//      method in the declaration of Foo, as in:
//
//          class Foo {
//          public:
//              Foo() ...;
//              Foo* nextInList() const { return mLink.next(); }
//          private:
//              TailQueueLink<Foo> mLink;
//              ...
//          };
//
//      Then use foo->nextInList() within the loop instead of foo->mLink.next()
//

// Template class for link fields inside objects that belong to a
// tail-queue list.
template <typename T>
class TailQueueLink {
public:
    TailQueueLink() : mNext(NULL), mPrevPointer(NULL) {}

    ~TailQueueLink() {}

    T* next() const { return mNext; }

    bool inQueue() const { return !!mPrevPointer; }

    bool isLast() const { return !mNext; }

    // These fields are public to make it easier for TailQueueList<>
    // to access them.
    T* mNext;
    T** mPrevPointer;
};

// Helper template class for tail-queue list instances.
// |T| is the type of objects that can be added to the list.
// |TRAITS| is the type of a structure that provides a single
// method that is:
//
//    static TailQueueLink<T>* link(T* t) const;
//
// Do not use this class directly, use the DECLARE_TAIL_QUEUE_LIST_CLASS
// macro instead, as defined below.
//
// Note that the implementation relies on the fact that the size and field
// layout of a TailQueueList<T,OFFSET> is identical to the one of a
// TailQueueListBase. This considerably reduces template instantiation
// bloat.
template <typename T, typename TRAITS = typename T::Traits>
class TailQueueList {
public:
    // Convenience typedef for link types.
    typedef TailQueueLink<T> LinkType;

    // Default constructor initializes an empty tail-queue list.
    TailQueueList() : mHead(NULL), mTail(&mHead) {}

    // Default destructor doesn't do anything.
    ~TailQueueList() {}

    // Returns truee iff the list is empty.
    bool empty() const {
        return mHead == NULL;
    }

    // Returns the address of the first item in the list, or NULL if it
    // is empty. This does not modify the list.
    T* front() const {
        return mHead;
    }

    // Returns the address of the last item in the list, or NULL if it
    // is empty. This does not modify the list.
    T* last() const {
        // Technical note: mTail points to either a link inside the last
        // item, or the mHead. Derefencing the value will point to the
        // last item in the tail queue list.
        return *(reinterpret_cast<TailQueueList*>(mTail)->mTail);
    }

    // Remove the first item from the list and return it to the caller.
    // Return NULL if the list was empty.
    T* popFront() {
        T* result = front();
        if (result) {
            remove(result);
        }
        return result;
    }

    // Append a new item to the list. This is a convenience alias for
    // insertTail().
    void pushBack(T* obj) {
        insertTail(obj);
    }

    // Prepend a new object |obj| to the list. Note that |obj| cannot be
    // NULL or already in another list using the same link.
    void insertHead(T* obj) {
        LinkType* link = TRAITS::link(obj);
        DCHECK(!link->inQueue());

        link->mNext = mHead;
        if (mHead) {
            LinkType* firstLink = TRAITS::link(mHead);
            firstLink->mPrevPointer = &link->mNext;
        } else {
            mTail = &link->mNext;
        }
        mHead = obj;
        link->mPrevPointer = &mHead;
    }

    // Append a new object |obj| to the list. Note that |obj| cannot be
    // NULL or already in another list using the same link.
    void insertTail(T* obj) {
        LinkType* link = TRAITS::link(obj);
        DCHECK(!link->inQueue());

        link->mNext = NULL;
        link->mPrevPointer = mTail;
        *mTail = obj;
        mTail = &link->mNext;
    }

    // Insert a new object |obj| into the list, before object |insert|.
    // Note that |obj| and |insert| cannot be NULL, |obj| must not be in
    // another list using the same link, and |insert| must already be
    // in the list.
    void insertBefore(T* insert, T* obj) {
        LinkType* link = TRAITS::link(obj);
        DCHECK(!link->inQueue());

        LinkType* insertLink = TRAITS::link(insert);
        DCHECK(insertLink->inQueue());

        link->mPrevPointer = insertLink->mPrevPointer;
        link->mNext = insert;
        *insertLink->mPrevPointer = obj;
        insertLink->mPrevPointer = &link->mNext;
    }

    // Insert a new object |obj| into the list, after object |insert|.
    // Note that |obj| and |insert| cannot be NULL, |obj| must not be in
    // another list using the same link, and |insert| must already be
    // in the list.
    void insertAfter(T* insert, T* obj) {
        LinkType* link = TRAITS::link(obj);
        DCHECK(!link->inQueue());

        LinkType* insertLink = TRAITS::link(insert);
        DCHECK(insertLink->inQueue());

        link->mNext = insertLink->mNext;
        if (link->mNext) {
            LinkType* nextLink = TRAITS::link(link->mNext);
            nextLink->mPrevPointer = &link->mNext;
        } else {
            mTail = &link->mNext;
        }
        insertLink->mNext = obj;
        link->mPrevPointer = &insertLink->mNext;
    }

    // Remove object |obj| from the list. |obj| cannot be NULL and must
    // be in the list.
    void remove(T* obj) {
        LinkType* link = TRAITS::link(obj);
        DCHECK(link->inQueue());

        if (link->mNext) {
            LinkType* nextLink = TRAITS::link(link->mNext);
            nextLink->mPrevPointer = link->mPrevPointer;
        } else {
            mTail = link->mPrevPointer;
        }
        *link->mPrevPointer = link->mNext;

        link->mNext = NULL;
        link->mPrevPointer = NULL;
    }

    // Return the number of objects in this list. WARNING: O(n) performance.
    size_t size() const {
        size_t count = 0;
        T* item = mHead;
        while (item) {
            count += 1;
            item = TRAITS::link(item)->mNext;
        }
        return count;
    }

    // Return true iff the list contains object |obj|.
    // WARNING: O(n) performance.
    bool contains(T* obj) const {
        T* item = mHead;
        while (item) {
            if (item == obj) {
                return true;
            }
            item = TRAITS::link(item)->mNext;
        }
        return false;
    }

    // Return the n-th item in the list, where |pos| is its position,
    // starting from 0. Return NULL if the position is out of bounds.
    // WARNING: O(n) performance.
    T* nth(size_t pos) const {
        T* item = mHead;
        while (item && pos > 0) {
            pos--;
            item = TRAITS::link(item)->mNext;
        }
        return item;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(TailQueueList);

    // The layout of these fields should match TailQueueListBase exactly.
    T* mHead;
    T** mTail;
};

#define TAIL_QUEUE_LIST_TRAITS(traits_name, type_name, field_name) \
    struct traits_name { \
        static inline ::android::base::TailQueueLink<type_name>* \
                link(type_name* t) { \
            return &(t)->field_name; \
        } \
    }

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_CONTAINERS_TAIL_QUEUE_LIST_H