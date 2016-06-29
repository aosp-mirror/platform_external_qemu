// Copyright 2015 The Android Open Source Project
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

#include "android/emulation/VmLock.h"

namespace qemu {

// A derived class of android::VmLock tailored for the QEMU1 main-loop.
class VmLock : public android::VmLock {
public:
    void lock() override;

    void unlock() override;

    bool isLockedBySelf() const override;
};

}  // namespace qemu
