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

#include "android/virtualscene/Texture.h"

namespace android {
namespace virtualscene {

RenderTarget::RenderTarget(const GLESv2Dispatch* gles2,
                           GLuint framebuffer,
                           GLuint depthRenderbuffer,
                           uint32_t width,
                           uint32_t height)
    : mGles2(gles2),
      mFramebuffer(framebuffer),
      mDepthRenderbuffer(depthRenderbuffer),
      mWidth(width),
      mHeight(height) {}

RenderTarget::~RenderTarget() {
    if (mGles2) {
        mGles2->glDeleteFramebuffers(1, &mFramebuffer);
        mGles2->glDeleteRenderbuffers(1, &mDepthRenderbuffer);
    }
}

void RenderTarget::setTexture(std::unique_ptr<Texture>&& texture) {
    mTexture = std::move(texture);
}

std::unique_ptr<RenderTarget> RenderTarget::createTextureTarget(
        const GLESv2Dispatch* gles2,
        uint32_t width,
        uint32_t height) {
    std::unique_ptr<Texture> texture =
            Texture::createEmpty(gles2, width, height);

    if (!texture) {
        return nullptr;
    }

    GLuint framebuffer;
    gles2->glGenFramebuffers(1, &framebuffer);
    gles2->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLuint depthRenderbuffer;
    gles2->glGenRenderbuffers(1, &depthRenderbuffer);
    gles2->glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    gles2->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                                  width, height);
    gles2->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER, depthRenderbuffer);

    gles2->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, texture->getTextureId(), 0);

    if (gles2->glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
        gles2->glDeleteFramebuffers(1, &framebuffer);
        gles2->glDeleteRenderbuffers(1, &depthRenderbuffer);
        return nullptr;
    }

    std::unique_ptr<RenderTarget> result(new RenderTarget(gles2,
            framebuffer, depthRenderbuffer, width, height));
    result->setTexture(std::move(texture));
    return result;
}

std::unique_ptr<RenderTarget> RenderTarget::createDefault(
        const GLESv2Dispatch* gles2, uint32_t width, uint32_t height) {
    return std::unique_ptr<RenderTarget>(new RenderTarget(gles2,
            0, 0, width, height));
}

void RenderTarget::bind() const {
    mGles2->glBindFramebuffer(GL_FRAMEBUFFER,
                              mFramebuffer);
    mGles2->glViewport(0, 0, mWidth, mHeight);
}

const Texture* RenderTarget::getTexture() const {
    return mTexture.get();
}

}  // namespace virtualscene
}  // namespace android
