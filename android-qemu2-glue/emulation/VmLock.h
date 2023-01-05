// Copyright 2016 The Android Open Source Project
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

#include "host-common/VmLock.h"

namespace qemu2 {

// An implementation of android::VmLock tailored for QEMU2
// Usage: install this in the current process with:
//
//      android::VmLock::set(new qemu2::VmLock());
//
class VmLock : public android::VmLock {
public:
    virtual void lock() override;
    virtual void unlock() override;
    virtual bool isLockedBySelf() const override;
};

}  // namespace qemu2
