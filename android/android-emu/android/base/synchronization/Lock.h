// Copyright (C) 2014 The Android Open Source Project
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

#include "android/base/Compiler.h"

namespace android {
namespace base {

class AutoLock;
class AutoWriteLock;
class AutoReadLock;

// A wrapper class for mutexes only suitable for using in static context,
// where it's OK to leak the underlying system object. Use Lock for scoped or
// member locks.
class StaticLock {
public:
    using AutoLock = android::base::AutoLock;
    StaticLock();
    void lock();
    bool tryLock();
    void unlock();
protected:
    friend class ConditionVariable;
    void* getPlatformLock();

    class Impl;
    Impl* mImpl;
    DISALLOW_COPY_ASSIGN_AND_MOVE(StaticLock);
};

// Simple wrapper class for mutexes used in non-static context.
class Lock : public StaticLock {
public:
    using StaticLock::AutoLock;
    Lock();
    ~Lock();
};

class ReadWriteLock {
public:
    using AutoWriteLock = android::base::AutoWriteLock;
    using AutoReadLock = android::base::AutoReadLock;
    ReadWriteLock();
    ~ReadWriteLock();
    void lockRead();
    void lockWrite();
    void unlockRead();
    void unlockWrite();
protected:
    void* getPlatformLock();

    class Impl;
    Impl* mImpl;
    DISALLOW_COPY_ASSIGN_AND_MOVE(ReadWriteLock);
};

// Helper class to lock / unlock a mutex automatically on scope
// entry and exit.
// NB: not thread-safe (as opposed to the Lock class)
class AutoLock {
public:
    AutoLock(StaticLock& lock);
    AutoLock(AutoLock&& other);
    ~AutoLock();

    void lock();
    void unlock();
    bool isLocked() const;

private:
    StaticLock& mLock;
    bool mLocked = true;
    friend class ConditionVariable;
    // Don't allow move because this class has a non-movable object.
    DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

class AutoWriteLock {
public:
    AutoWriteLock(ReadWriteLock& lock);
    ~AutoWriteLock();
    void lockWrite();
    void unlockWrite();
private:
    ReadWriteLock& mLock;
    bool mWriteLocked = true;
    // This class has a non-movable object.
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoWriteLock);
};

class AutoReadLock {
public:
    AutoReadLock(ReadWriteLock& lock);
    ~AutoReadLock();
    void lockRead();
    void unlockRead();
private:
    ReadWriteLock& mLock;
    bool mReadLocked = true;
    // This class has a non-movable object.
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoReadLock);
};

}  // namespace base
}  // namespace android
