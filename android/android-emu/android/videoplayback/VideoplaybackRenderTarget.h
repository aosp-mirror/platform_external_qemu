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

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/recording/video/player/VideoPlayerRenderTarget.h"

#include <array>
#include <memory>

namespace android {
namespace videoplayback {

class VideoplaybackRenderTarget : public VideoPlayerRenderTarget {
public:
    VideoplaybackRenderTarget(const GLESv2Dispatch* gles2,
                              int width,
                              int height);
    void getRenderTargetSize(float sampleAspectRatio,
                             int video_width,
                             int video_height,
                             int* resultRenderTargetWidth,
                             int* resultRenderTargetHeight) override;
    void setPixelBuffer(const FrameInfo& info,
                        const unsigned char* buf,
                        size_t len) override;

    bool initialize();
    void uninitialize();
    void renderBuffer();

private:
    void renderVideoFrame();
    void renderEmptyFrame();
    const unsigned char* mBuffer = nullptr;
    size_t mBufferLen = 0;
    FrameInfo mFrameInfo;
    const GLESv2Dispatch* const mGles2;
    size_t mRenderWidth;
    size_t mRenderHeight;
    bool mInitialized = false;

    GLuint mVertexBuffer;
    GLuint mElementBuffer;
    GLuint mVertexShader;
    GLuint mFragmentShader;
    GLuint mProgram;
    GLuint mVao;
    std::array<GLuint, 2> mTextures;
};

}  // namespace videoplayback
}  // namespace android
