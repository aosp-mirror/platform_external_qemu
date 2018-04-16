/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "android/virtualscene/RenderTarget.h"

namespace android {
namespace virtualscene {

RenderTarget::RenderTarget(Renderer& renderer,
                           const GLESv2Dispatch* gles2,
                           GLuint framebuffer,
                           GLuint depthRenderbuffer,
                           Texture texture,
                           uint32_t width,
                           uint32_t height)
    : mRenderer(renderer),
      mGles2(gles2),
      mFramebuffer(framebuffer),
      mDepthRenderbuffer(depthRenderbuffer),
      mTexture(texture),
      mWidth(width),
      mHeight(height) {}

RenderTarget::~RenderTarget() {
    mRenderer.releaseTexture(mTexture);

    if (mGles2) {
        mGles2->glDeleteFramebuffers(1, &mFramebuffer);
        mGles2->glDeleteRenderbuffers(1, &mDepthRenderbuffer);
    }
}

std::unique_ptr<RenderTarget> RenderTarget::createTextureTarget(
        Renderer& renderer,
        const GLESv2Dispatch* gles2,
        GLuint textureId,
        Texture texture,
        uint32_t width,
        uint32_t height) {
    GLuint framebuffer;
    gles2->glGenFramebuffers(1, &framebuffer);
    gles2->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLuint depthRenderbuffer;
    gles2->glGenRenderbuffers(1, &depthRenderbuffer);
    gles2->glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    gles2->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width,
                                 height);
    gles2->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                     GL_RENDERBUFFER, depthRenderbuffer);

    gles2->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, textureId, 0);

    if (gles2->glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
        gles2->glDeleteFramebuffers(1, &framebuffer);
        gles2->glDeleteRenderbuffers(1, &depthRenderbuffer);
        return nullptr;
    }

    std::unique_ptr<RenderTarget> result(
            new RenderTarget(renderer, gles2, framebuffer, depthRenderbuffer,
                             texture, width, height));
    return result;
}

std::unique_ptr<RenderTarget> RenderTarget::createDefault(
        Renderer& renderer,
        const GLESv2Dispatch* gles2,
        uint32_t width,
        uint32_t height) {
    return std::unique_ptr<RenderTarget>(
            new RenderTarget(renderer, gles2, 0, 0, Texture(), width, height));
}

void RenderTarget::bind() const {
    mGles2->glBindFramebuffer(GL_FRAMEBUFFER,
                              mFramebuffer);
    mGles2->glViewport(0, 0, mWidth, mHeight);
}

const Texture& RenderTarget::getTexture() const {
    return mTexture;
}

}  // namespace virtualscene
}  // namespace android
