/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "android/camera/camera-videoplayback-render-multiplexer.h"

#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/offworld/proto/offworld.pb.h"

namespace android {
namespace videoplayback {

bool RenderMultiplexer::initialize(const GLESv2Dispatch* gles2,
                                   int width,
                                   int height) {
    mDefaultRenderer =
            std::unique_ptr<DefaultFrameRenderer>(new DefaultFrameRenderer());
    if (!mDefaultRenderer->initialize(gles2, width, height)) {
        uninitialize();
        return false;
    }
    mCurrentRenderer = mDefaultRenderer.get();
    mInitialized = true;
    return true;
}

void RenderMultiplexer::uninitialize() {
    mCurrentRenderer = nullptr;
    mDefaultRenderer.reset();
    mInitialized = false;
}

int64_t RenderMultiplexer::render() {
    if (!mInitialized) {
        LOG(ERROR) << "Attempting to render using an uninitialized render "
                      "multiplexer";
        return -1;
    }

    base::Optional<::offworld::VideoInjectionRequest> maybe_next_request =
            videoinjection::VideoInjectionController::tryGetNextRequest(
                    base::Ok());
    if (maybe_next_request) {
        switch (maybe_next_request->function_case()) {
            case ::offworld::VideoInjectionRequest::kDisplayDefaultFrame:
              mCurrentRenderer = mDefaultRenderer.get();
              break;
            default:
              mCurrentRenderer = mDefaultRenderer.get();
              break;
        }
    }
    return mCurrentRenderer->render();
}

}  // namespace videoplayback
}  // namespace android
