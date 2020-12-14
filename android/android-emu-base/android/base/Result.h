// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/Optional.h"

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
//   if (result.ok()) { auto value = *result.ok(); }  // no error
//   return result;  // propagate result as-is
//   RETURN_IF_ERR(result);  // propagate error
//   switch (*result.err()) { ... }  // handle error
//
//   auto result = function();
//   RETURN_IF_ERR(result);   // propagate error

// Helper to propagate errors.
#define RETURN_IF_ERR(...)                                     \
    do {                                                       \
        auto&& __result = __VA_ARGS__;                         \
        if (__result.err()) {                                  \
            return ::android::base::Err(__result.unwrapErr()); \
        }                                                      \
    } while (false)

namespace android {
namespace base {

namespace detail {

template <typename T>
struct Ok {
    Ok(const T& value) : value(value) {}
    Ok(T&& value) : value(std::move(value)) {}

    Ok(Ok&& other) : value(std::move(other.value)) {}

    T value;
};

template <>
struct Ok<void> {
    Ok() = default;
};

template <typename E>
struct Err {
public:
    Err(const E& value) : value(value) {}
    Err(E&& value) : value(std::move(value)) {}

    Err(Err&& other) : value(std::move(other.value)) {}

    E value;
};

template <typename T, typename E>
struct ResultStorage {
    template <typename U,
              typename = typename std::enable_if<
                      std::is_convertible<T, U>::value>::type>
    ResultStorage(Ok<U>&& ok) : ok(std::move(ok.value)) {}

    ResultStorage(Err<E>&& err) : err(std::move(err.value)) {}

    bool isOk() const { return ok.hasValue(); }

    Optional<T> ok;
    Optional<E> err;
};

template <typename E>
struct ResultStorage<void, E> {
    ResultStorage(Ok<void>&& ok) {}
    ResultStorage(Err<E>&& err) : err(std::move(err.value)) {}

    bool isOk() const { return !err.hasValue(); }

    Optional<E> err;
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

    Result(Result&& other)
        : mStorage(std::move(other.mStorage)), mValid(other.mValid) {
        other.mValid = false;
    }

    // Returns an Optional<T> representing the value, not defined is T is void.
    template <typename U = T>
    typename std::enable_if<!std::is_void<U>::value &&
                                    std::is_copy_constructible<U>::value,
                            Optional<U>>::type
    ok() {
        CHECK(mValid) << "Result invalid";
        return mStorage.ok;
    }
    template <typename U = T>
    typename std::enable_if<!std::is_void<U>::value, const Optional<U>&>::type
    ok() const {
        CHECK(mValid) << "Result invalid";
        return mStorage.ok;
    }

    // For Result<void, E> types, returns true if the Result is ok.
    template <typename U = T>
    typename std::enable_if<std::is_void<U>::value, bool>::type ok() const {
        CHECK(mValid) << "Result invalid";
        return mStorage.isOk();
    }

    // Returns an Optional<E> representing the error, if it exists.
    template <typename U = E>
    typename std::enable_if<std::is_copy_constructible<U>::value,
                            Optional<U>>::type
    err() {
        CHECK(mValid) << "Result invalid";
        return mStorage.err;
    }
    const Optional<E>& err() const {
        CHECK(mValid) << "Result invalid";
        return mStorage.err;
    }

    // Unwraps the value and returns it.  After this call the Result is invalid.
    template <typename U = T>
    typename std::enable_if<!std::is_void<U>::value, U>::type unwrap() {
        CHECK(mValid) << "Result invalid";
        mValid = false;
        return std::move(*(mStorage.ok.ptr()));
    }

    // Unwraps the error and returns it.  After this call the Result is invalid.
    E unwrapErr() {
        CHECK(mValid) << "Result invalid";
        mValid = false;
        return std::move(*(mStorage.err.ptr()));
    }

private:
    detail::ResultStorage<T, E> mStorage;
    bool mValid = true;
};

}  // namespace base
}  // namespace android
