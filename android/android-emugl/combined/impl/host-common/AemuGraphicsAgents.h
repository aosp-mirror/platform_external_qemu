// Copyright (C) 2022 The Android Open Source Project
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

#include <stdio.h>

#include "host-common/GraphicsAgentFactory.h"
#include "host-common/hw-config.h"

namespace android {
namespace emulation {

// Creates a series of AEMU Agents that can be used by GfxStream backend.
class AemuGraphicsAgentFactory : public GraphicsAgentFactory {
public:
    const QAndroidEmulatorWindowAgent* const
        android_get_QAndroidEmulatorWindowAgent() const override;

    const QAndroidDisplayAgent* const
        android_get_QAndroidDisplayAgent() const override;

    const QAndroidRecordScreenAgent* const
        android_get_QAndroidRecordScreenAgent() const override;

    const QAndroidMultiDisplayAgent* const
        android_get_QAndroidMultiDisplayAgent() const override;

    const QAndroidVmOperations* const
        android_get_QAndroidVmOperations() const override;
};

}  // namespace emulation
}  // namespace android
