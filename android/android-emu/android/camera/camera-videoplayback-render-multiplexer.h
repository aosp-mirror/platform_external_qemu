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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <iosfwd>
#include <memory>
#include <string>

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/Optional.h"
#include "android/utils/compiler.h"
#include "android/recording/video/player/VideoPlayer.h"
#include "android/videoinjection/VideoInjectionController.h"
#include "android/videoplayback/VideoplaybackRenderTarget.h"
#include "android/camera/camera-videoplayback-default-renderer.h"
#include "android/camera/camera-videoplayback-video-renderer.h"
#include "android/camera/camera-virtualscene-utils.h"
#include "android/recording/video/player/VideoPlayer.h"
#include "android/utils/compiler.h"
#include "android/utils/tempfile.h"
#include "android/videoinjection/VideoInjectionController.h"
#include "android/videoplayback/VideoplaybackRenderTarget.h"
#include "offworld.pb.h"

struct GLESv2Dispatch;

using namespace android::base;
typedef ::offworld::DatasetInfo DatasetInfo;

namespace android {
namespace videoplayback {

// Render Multiplexer chooses which renderer to use for the Virtual Scene camera
// based on incoming requests from the Video Injection Controller. If no
// requests are made, the Render Multiplexer uses the most recent renderer
// requested. The Render Multiplexer starts with the Default Frame Renderer as
// its initial renderer.
class RenderMultiplexer : public virtualscene::CameraRenderer {
public:
    RenderMultiplexer() = default;
    ~RenderMultiplexer() = default;
    bool initialize(const GLESv2Dispatch* gles2,
                    int width,
                    int height) override;
    void uninitialize() override;
    int64_t render() override;

private:
    void loadVideo(const std::string& video_data, uint32_t async_id);
    void loadVideoWithData(const std::string& video_data,
                           const DatasetInfo& datasetInfo,
                           uint32_t async_id);
    void switchRenderer(virtualscene::CameraRenderer* renderer);
    bool videoIsLoaded(uint32_t async_id);

    std::unique_ptr<DefaultFrameRenderer> mDefaultRenderer;
    std::unique_ptr<VideoplaybackVideoRenderer> mVideoRenderer;
    std::unique_ptr<videoplayer::VideoPlayer> mPlayer;
    virtualscene::CameraRenderer* mCurrentRenderer = nullptr;
    bool mInitialized = false;
    size_t mCounter = 0;
    // async_id for ongoing non-sequential execution, including but not limited
    // to play, pause.
    Optional<uint32_t> mOngoingAsyncId;
    bool mPlaysData = false;

    TempFile* mTempfile = nullptr;
};

}  // namespace videoplayback
}  // namespace android
