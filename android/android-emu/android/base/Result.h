// Copyright 2018 The Android Open Source Project
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

#include "android/base/Optional.h"
#include "android/base/Log.h"

// Result<T, E> - a template class to store either a result or error, inspired
//                by Rust.
//
// T is the successful result type, and may be void.
// E is the error result type.
//
// To return success, return Ok(value) or Ok().
// To return error, return Err(value).
//
// Usage in APIs:
//
//   class MyError1 { Error1, Error2 };
//   typedef Result<void, MyError> MyResult;
//
//   MyResult function() {
//     if (good()) return Ok();
//     else return Err(MyError::Error1);
//   }
//
// Checking results:
//
//   Result<uint32_t, MyError> result = function2();  // may also be "auto".
//
//   if (result.isOk()) { ... }  // ignore error
//   if (result.isErr()) { return result.err() }  // propogate error
//
//   function().unwrap();  // returns success value or crashes.

namespace android {
namespace base {

namespace detail {

template <typename T>
struct Ok {
    Ok(const T& value) : mValue(value) {}
    Ok(T&& value) : mValue(std::move(value)) {}

    Ok(Ok&& other) : mValue(std::move(other.mValue)) {}

    T mValue;
};

template <>
struct Ok<void> {
    Ok() = default;
};

template <typename E>
struct Err {
public:
    Err(const E& value) : mValue(value) {}
    Err(E&& value) : mValue(std::move(value)) {}

    Err(Err&& other) : mValue(std::move(other.mValue)) {}

    E mValue;
};

template <typename T, typename E>
struct ResultStorage {
    template <typename U,
              typename = typename std::enable_if<
                      std::is_convertible<T, U>::value>::type>
    ResultStorage(Ok<U>&& ok) : mOk(std::move(ok.mValue)) {}

    ResultStorage(Err<E>&& err) : mErr(std::move(err.mValue)) {}

    bool isOk() const { return mOk.hasValue(); }

    Optional<T> mOk;
    Optional<E> mErr;
};

template <typename E>
struct ResultStorage<void, E> {
    ResultStorage(Ok<void>&& ok) {}
    ResultStorage(Err<E>&& err) : mErr(std::move(err.mValue)) {}

    bool isOk() const { return !mErr.hasValue(); }

    Optional<E> mErr;
};

}  // namespace detail

template <typename T, typename Value = typename std::decay<T>::type>
detail::Ok<Value> Ok(T&& value) {
    return detail::Ok<Value>(std::forward<T>(value));
}

static inline detail::Ok<void> Ok() {
    return detail::Ok<void>();
}

template <typename E, typename Value = typename std::decay<E>::type>
detail::Err<Value> Err(E&& value) {
    return detail::Err<Value>(std::forward<E>(value));
}

template <typename T, typename E>
class Result {
public:
    template <typename U,
              typename = typename std::enable_if<
                      std::is_convertible<U, T>::value>::type>
    Result(detail::Ok<U>&& ok) : mStorage(std::move(ok)) {}
    Result(detail::Err<E>&& err) : mStorage(std::move(err)) {}

    // Returns true if the object has an Ok value.
    bool isOk() const { return mStorage.isOk(); }

    // Returns true if the object has an Err value.
    bool isErr() const { return mStorage.mErr.hasValue(); }

    // Returns an Optional<T> representing the value, not defined is T is void.
    template <typename U = T>
    typename std::enable_if<!std::is_void<U>::value &&
                                    std::is_copy_constructible<U>::value,
                            Optional<U>>::type
    ok() { return mStorage.mOk; }
    template <typename U = T>
    typename std::enable_if<!std::is_void<U>::value, const Optional<U>&>::type
    ok() const { return mStorage.mOk; }

    // Returns an Optional<E> representing the error, if it exists.
    template <typename U = E>
    typename std::enable_if<std::is_copy_constructible<U>::value,
                            Optional<U>>::type
    err() { return mStorage.mErr; }
    const Optional<E>& err() const { return mStorage.mErr; }

    // Unwraps the value, or crashes if there is an error.
    T unwrap() {
        CHECK(isOk()) << "Result::unwrap failed";
        return ok().value();
    }

    // Unwraps the value, or crashes if there is no error.
    E unwrapErr() {
        CHECK(isErr()) << "Result::unwrapErr failed";
        return err().value();
    }

private:
    detail::ResultStorage<T, E> mStorage;
};

}  // namespace base
}  // namespace android
