// Copyright 2015-2016 The Android Open Source Project
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

#include "android/base/TypeUtils.h"

#include <initializer_list>
#include <type_traits>
#include <utility>

#include <stddef.h>

// Optional<T> - a template class for passing through an optional value;
// it's basically a stack-stored object with a pointer semantic.
// Use it when you need to return a single value from a function or indicate
// an error, e.g.:
//
// Optional<int> socket(...) {
//     int sock = ...;
//     if (sock < 0) {
//         return kNullopt;
//     }
//     return sock;
// }
//
// The class supports a set of conversions one would expect from it:
//  - it can be constructed from anything that's convertible to the type |T|
//  - the |T| object can be constructed in-place with syntax:
//      Optional<T>(kInplace, <args>);
//  - a convenience function makeOptional() would deduce its type for you
//  - it has an operator bool() which returns true if there's a value in the
//      optional object
//  - overloaded operators * and -> make the syntax very poiner-like
//  - valueOr() provides one a convenience getter:
//      Optional<int> size = ...;
//      vector.resize(size.valueOr(100));
//  - operators == and < compare optionals to optionals and unpacked values
//      (empty optional is smaller than any object of type |T|)

namespace android {
namespace base {

namespace details {

// A base class to reduce the number of instantiations of the Optional's
// internal storage
template <size_t Size, size_t Align>
class OptionalBase {
protected:
    using StoreT = typename std::aligned_storage<Size, Align>::type;

    StoreT mStorage;

    void setConstructed(bool constructed) { mConstructed = constructed; }
    constexpr bool constructed() const { return mConstructed; }
    constexpr operator bool() const { return constructed(); }

    constexpr OptionalBase(bool constructed = false)
        : mConstructed(constructed) {}

private:
    bool mConstructed;
};

}  // namespace details

// A tag type for empty optional construction
struct NulloptT {
    constexpr explicit NulloptT(int) {}
};

// A tag type for inplace value construction
struct InplaceT {
    constexpr explicit InplaceT(int) {}
};


// Tag values for null optional and inplace construction
constexpr NulloptT kNullopt{1};
constexpr InplaceT kInplace{1};

template <class T>
class Optional : private details::OptionalBase<sizeof(T),
                                               std::alignment_of<T>::value> {
    // make sure all optionals are buddies - this is needed to implement
    // conversion from optionals of other types
    template <class U> friend class Optional;

    using base_type = details::OptionalBase<sizeof(T), std::alignment_of<T>::value>;

public:
    // std::optional will have this, so let's provide it
    using value_type = T;

    constexpr Optional() {}
    constexpr Optional(NulloptT) {}

    Optional(const Optional& other) : base_type(other.constructed()) {
        if (this->constructed()) {
            new (&get()) T(other.get());
        }
    }
    Optional(Optional&& other) : base_type(other.constructed()) {
        if (this->constructed()) {
            new (&get()) T(std::move(other.get()));
        }
    }

    // Conversion constructor from optional of similar type
    template <class U,
              class = enable_if_c<
                  !is_template_instantiation<typename std::remove_reference<
                                                  typename std::remove_cv<U>::type
                                              >::type,
                                              Optional>::value
                  && std::is_constructible<T, U>::value>>
    Optional(const Optional<U>& other) : base_type(other.constructed()) {
        if (this->constructed()) {
            new (&get()) T(other.get());
        }
    }

    // Move-conversion constructor
    template <class U,
              class = enable_if_c<
                  !is_template_instantiation<typename std::remove_reference<
                                                  typename std::remove_cv<U>::type
                                              >::type,
                                              Optional>::value
                  && std::is_constructible<T, U>::value>>
    Optional(Optional<U>&& other) : base_type(other.constructed()) {
        if (this->constructed()) {
            new (&get()) T(std::move(other.get()));
        }
    }

    // Construction from a raw value
    Optional(const T& value) : base_type(true) {
        new (&get()) T(value);
    }
    // Move construction from a raw value
    Optional(T&& value) : base_type(true) {
        new (&get()) T(std::move(value));
    }

    // Inplace construction from a list of |T|'s ctor arguments
    template <class... Args>
    Optional(InplaceT, Args&&... args) : base_type(true) {
        new (&get()) T(std::forward<Args>(args)...);
    }

    // Inplace construction from an initializer list passed into |T|'s ctor
    template <class U,
              class = enable_if<std::is_constructible<T, std::initializer_list<U>>>>
    Optional(InplaceT, std::initializer_list<U> il) : base_type(true) {
        new (&get()) T(il);
    }

    // direct assignment
    Optional& operator=(const Optional& other) {
        if (&other == this) {
            return *this;
        }

        if (this->constructed()) {
            if (other.constructed()) {
                get() = other.get();
            } else {
                destruct();
                this->setConstructed(false);
            }
        } else {
            if (other.constructed()) {
                new (&get()) T(other.get());
                this->setConstructed(true);
            } else {
                ;  // we're good
            }
        }
        return *this;
    }

    // move assignment
    Optional& operator=(Optional&& other) {
        if (this->constructed()) {
            if (other.constructed()) {
                get() = std::move(other.get());
            } else {
                destruct();
                this->setConstructed(false);
            }
        } else {
            if (other.constructed()) {
                new (&get()) T(std::move(other.get()));
                this->setConstructed(true);
            } else {
                ;  // we're good
            }
        }
        return *this;
    }

    // conversion assignment
    template <class U,
              class = enable_if_convertible<typename std::decay<U>::type, T>>
    Optional& operator=(const Optional<U>& other) {
        if (this->constructed()) {
            if (other.constructed()) {
                get() = other.get();
            } else {
                destruct();
                this->setConstructed(false);
            }
        } else {
            if (other.constructed()) {
                new (&get()) T(other.get());
                this->setConstructed(true);
            } else {
                ;  // we're good
            }
        }
        return *this;
    }

    // conversion move assignment
    template <class U,
              class = enable_if_convertible<typename std::decay<U>::type, T>>
    Optional& operator=(Optional<U>&& other) {
        if (this->constructed()) {
            if (other.constructed()) {
                get() = std::move(other.get());
            } else {
                destruct();
                this->setConstructed(false);
            }
        } else {
            if (other.constructed()) {
                new (&get()) T(std::move(other.get()));
                this->setConstructed(true);
            } else {
                ;  // we're good
            }
        }
        return *this;
    }

    // the most complicated one: forwarding constructor for anything convertible
    // to |T|, excluding the stuff implemented above explicitly
    template <class U,
              class = enable_if_c<
                  !is_template_instantiation<typename std::remove_reference<
                                                  typename std::remove_cv<U>::type
                                              >::type,
                                              Optional>::value
                  && std::is_convertible<typename std::decay<U>::type, T>::value>>
    Optional& operator=(U&& other) {
        if (this->constructed()) {
            get() = std::forward<U>(other);
        } else {
            new (&get()) T(std::forward<U>(other));
            this->setConstructed(true);
        }
        return *this;
    }

    // Adopt operator bool() from the parent
    using base_type::operator bool;

    T& value() { return get(); }
    constexpr const T& value() const { return get(); }

    // Value getter with fallback
    template <class U,
              class = enable_if_convertible<typename std::decay<U>::type, T>>
    constexpr T valueOr(U&& defaultValue) const {
        return this->constructed() ? get() : std::move(defaultValue);
    }

    // Pointer-like operators
    T& operator*() { return get(); }
    constexpr const T& operator*() const { return get(); }

    T* operator->() { return &get(); }
    constexpr const T* operator->() const { return &get(); }


    ~Optional() {
        if (this->constructed()) {
            destruct();
        }
    }

    void clear() {
        if (this->constructed()) {
            destruct();
            this->setConstructed(false);
        }
    }

    template <class U,
              class = enable_if_convertible<typename std::decay<U>::type, T>>
    void reset(U&& u) {
        *this = std::forward<U>(u);
    }

    // In-place construction with possible destruction of the old value
    template <class... Args>
    void emplace(Args&&... args) {
        if (this->constructed()) {
            destruct();
        }
        new (&get()) T(std::forward<Args>(args)...);
    }

    // In-place construction with possible destruction of the old value
    // initializer-list version
    template <class U,
              class = enable_if<std::is_constructible<T, std::initializer_list<U>>>>
    void emplace(std::initializer_list<U> il) {
        if (this->constructed()) {
            destruct();
        }
        new (&get()) T(il);
    }

private:
    // A helper function to convert the internal raw storage to T&
    constexpr const T& get() const {
        return *reinterpret_cast<const T*>(
                    reinterpret_cast<const char*>(&this->mStorage));
    }

    // Same thing, mutable
    T& get() {
        return const_cast<T&>(const_cast<const Optional*>(this)->get());
    }

    // Shortcut for a destructor call for the stored object
    void destruct() { get().T::~T(); }
};

template <class T>
Optional<typename std::decay<T>::type> makeOptional(T&& t) {
    return Optional<typename std::decay<T>::type>(std::forward<T>(t));
}

template <class T>
bool operator==(const Optional<T>& l, const Optional<T>& r) {
    return bool(l)
            ? bool(r) && *l == *r
            : !bool(r);
}
template <class T>
bool operator==(const Optional<T>& l, NulloptT) {
    return !l;
}
template <class T>
bool operator==(NulloptT, const Optional<T>& r) {
    return !r;
}
template <class T>
bool operator==(const Optional<T>& l, const T& r) {
    return bool(l) && *l == r;
}
template <class T>
bool operator==(const T& l, const Optional<T>& r) {
    return bool(r) && l == *r;
}

template <class T>
bool operator!=(const Optional<T>& l, const Optional<T>& r) {
    return !(l == r);
}
template <class T>
bool operator!=(const Optional<T>& l, NulloptT) {
    return bool(l);
}
template <class T>
bool operator!=(NulloptT, const Optional<T>& r) {
    return bool(r);
}
template <class T>
bool operator!=(const Optional<T>& l, const T& r) {
    return !l || !(*l == r);
}
template <class T>
bool operator!=(const T& l, const Optional<T>& r) {
    return !r || !(l == *r);
}

template <class T>
bool operator<(const Optional<T>& l, const Optional<T>& r) {
    return !r ? false : (!l ? true : *l < *r);
}
template <class T>
bool operator<(const Optional<T>&, NulloptT) {
    return false;
}
template <class T>
bool operator<(NulloptT, const Optional<T>& r) {
    return bool(r);
}
template <class T>
bool operator<(const Optional<T>& l, const T& r) {
    return !l || *l < r;
}
template <class T>
bool operator<(const T& l, const Optional<T>& r) {
    return bool(r) && l < *r;
}

}  // namespace base
}  // namespace android
