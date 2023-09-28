// Copyright (C) 2023 The Android Open Source Project
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
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

namespace android {
namespace emulation {
namespace control {

/**
 * @brief A library class for managing borrowed objects with thread safety.
 *
 * The `Library` class provides a mechanism for borrowing and returning objects
 * of type `T` while ensuring thread safety using a mutex. Borrowed objects are
 * managed in an `std::unordered_set` to allow for efficient lookup and removal.
 * You can iterate over the objects with the thread safe forEach method.
 *
 * @tparam T The type of objects to be borrowed and managed.
 */
template <typename T>
class Library {
public:
    /**
     * @brief Borrow an object of type `T` with optional constructor arguments.
     *
     * This method creates a new shared pointer to an object of type `T` and
     * adds it to the library's borrowed objects collection. The shared
     * pointer's custom deleter automatically removes the object from the
     * collection when its reference count reaches 0.
     *
     * You can provide optional constructor arguments for the object of type
     * `T`, and these arguments will be forwarded to the object's constructor.
     *
     * Example usage:
     * ```cpp
     * Library<MyClass> myLibrary;
     * auto ptr = myLibrary.acquire(arg1, arg2, ...);
     * arguments
     * ```
     *
     * @tparam Args Parameter pack for the constructor arguments of type `T`.
     * @param args Constructor arguments for the object of type `T`.
     * @return A shared pointer to the borrowed object.
     */
    template <typename... Args>
    std::shared_ptr<T> acquire(Args&&... args) {
        std::shared_ptr<T> obj(new T(std::forward<Args>(args)...),
                               [this](T* ptr) {
                                   removeBorrowedObject(ptr);
                                   delete ptr;
                               });

        std::lock_guard<std::mutex> lock(mBorrowedLock);
        mBorrowed.insert(obj.get());

        return obj;
    }

    /**
     * @brief Apply the function fun to each borrowed object.
     *
     * @param fun The function to apply to each borrowed object's pointer.
     */
    void forEach(std::function<void(T*)> fun) {
        std::lock_guard<std::mutex> lock(mBorrowedLock);
        for (auto ptr : mBorrowed) {
            fun(ptr);
        }
    }

    // Wait until the library's borrowed objects are all returned, or until a
    // timeout occurs. returns `true` if the library is empty within the
    // timeout, `false` otherwise.
    bool waitUntilLibraryIsClear(const std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mBorrowedLock);
        return mBorrowedCv.wait_for(lock, timeout,
                                    [this]() { return mBorrowed.empty(); });
    }

private:
    std::unordered_set<T*> mBorrowed;
    std::mutex mBorrowedLock;
    std::condition_variable mBorrowedCv;

    // Safely removes a borrowed object, called upon destruction of object.
    void removeBorrowedObject(T* object) {
        std::lock_guard<std::mutex> lock(mBorrowedLock);
        mBorrowed.erase(object);
        mBorrowedCv.notify_all();
    }
};
}  // namespace control
}  // namespace emulation
}  // namespace android