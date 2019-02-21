/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Optional.h"
#include "android/base/Result.h"
#include "android/base/memory/LazyInstance.h"
#include "android/utils/compiler.h"
#include "android/videoinjection/VideoInjectionController.h"

#include <memory>
#include <vector>

namespace android {
namespace videoplayback {

class DefaultFrameRendererImpl {
public:
    DefaultFrameRendererImpl(const GLESv2Dispatch* gles2,
                             int width,
                             int height);
    ~DefaultFrameRendererImpl() = default;

    bool initialize();
    void render();

private:
    const GLESv2Dispatch* const mGles2;
    const int mRenderWidth;
    const int mRenderHeight;
    bool displayDefault;
};

}  // namespace videoplayback
}  // namespace android
