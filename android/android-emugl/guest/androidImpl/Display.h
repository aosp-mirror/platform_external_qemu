// Copyright 2018 The Android Open Source Project
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

#include "AndroidBufferQueue.h"
#include "GrallocDispatch.h"
#include "HardwareComposer.h"
#include "SurfaceFlinger.h"

#include <memory>

namespace aemu {

class Display {
public:
    Display();

private:
    class Vsync;

    struct gralloc_implementation mGralloc;

    AndroidBufferQueue mSf2Hwc;
    AndroidBufferQueue mHwc2Sf;

    std::unique_ptr<HardwareComposer> mHwc;
    std::unique_ptr<SurfaceFlinger> mSf;
};

} // namespace aemu