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

#include "android/videoplayback/VideoplaybackRenderTarget.h"

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Log.h"
#include "android/base/Optional.h"

#include <algorithm>
#include <vector>

namespace android {
namespace videoplayback {

// Shader Utilities
// Shaders we need to define to properly render the video frame, and the
// accompanying utility functions to use them.

static const char kFrameTextureVertexShader[] = R"(
attribute vec2 in_position;
attribute vec2 in_texcoord;

varying vec2 texcoord;

void main() {
    texcoord = in_texcoord;
    gl_Position = vec4(in_position, 0.0, 1.0);
}
)";

static const char kFrameTextureFragmentShader[] = R"(
precision mediump float;

varying vec2 texcoord;

uniform sampler2D frameTexture;

void main() {
    gl_FragColor = texture2D(frameTexture, texcoord);
}
)";

static GLuint compileShader(const GLESv2Dispatch* const gles2,
                            GLenum type,
                            const char* shaderSource) {
    const GLuint shaderId = gles2->glCreateShader(type);
    gles2->glShaderSource(shaderId, 1, &shaderSource, nullptr);
    gles2->glCompileShader(shaderId);

    int logLength = 0;
    GLint result = GL_FALSE;
    gles2->glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) {
        gles2->glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength) {
            std::vector<char> message(logLength + 1);
            gles2->glGetShaderInfoLog(shaderId, logLength, nullptr,
                                      message.data());
            LOG(ERROR) << "Failed to compile shader: " << message.data();
        } else {
            LOG(ERROR) << "Failed to compile shader.";
        }
        gles2->glDeleteShader(shaderId);
        return 0;
    }
    return shaderId;
}

// VIDEOPLAYBACK RENDER TARGET
VideoplaybackRenderTarget::VideoplaybackRenderTarget(
        const GLESv2Dispatch* gles2,
        int width,
        int height)
    : mGles2(gles2), mRenderWidth(width), mRenderHeight(height) {}

void VideoplaybackRenderTarget::getRenderTargetSize(
        float sampleAspectRatio,
        int video_width,
        int video_height,
        int* resultRenderTargetWidth,
        int* resultRenderTargetHeight) {
    // Attempt to clamp to mRenderHeight first, then mRenderWidth.
    int height = mRenderHeight;
    int width = static_cast<int>(height * sampleAspectRatio + 0.5f);
    if (width > mRenderWidth) {
        width = mRenderWidth;
        height = static_cast<int>(width / sampleAspectRatio + 0.5f);
    }
    *resultRenderTargetWidth = width;
    *resultRenderTargetHeight = height;
}

void VideoplaybackRenderTarget::setPixelBuffer(const FrameInfo& info,
                                               const unsigned char* buf,
                                               size_t len) {
    mFrameInfo = info;
    mBuffer = buf;
    mBufferLen = len;
}

bool VideoplaybackRenderTarget::initialize() {
    if (mInitialized) {
      return true;
    }

    // Vertices necessary to draw a rectangle (i.e. two triangles).
    GLfloat vertices[] = {
            // Position   Tex Coords
            -1.0f, 1.0f,  0.0f, 1.0f,  // Top-left
            1.0f,  1.0f,  1.0f, 1.0f,  // Top-right
            1.0f,  -1.0f, 1.0f, 0.0f,  // Bottom-right
            -1.0f, -1.0f, 0.0f, 0.0f,  // Bottom-left
    };

    GLuint elements[] = {0, 1, 2, 2, 3, 0};

    mGles2->glGenVertexArrays(1, &mVao);
    mGles2->glBindVertexArray(mVao);

    mGles2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mGles2->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mGles2->glViewport(0, 0, mRenderWidth, mRenderHeight);
    mGles2->glFrontFace(GL_CW);

    mGles2->glGenTextures(1, &mTexture);
    mGles2->glBindTexture(GL_TEXTURE_2D, mTexture);

    mGles2->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mRenderWidth, mRenderHeight,
                         0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    mGles2->glGenBuffers(1, &mVertexBuffer);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    mGles2->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                         GL_STATIC_DRAW);

    mGles2->glGenBuffers(1, &mElementBuffer);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffer);
    mGles2->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements,
                         GL_STATIC_DRAW);

    mVertexShader =
            compileShader(mGles2, GL_VERTEX_SHADER, kFrameTextureVertexShader);
    mFragmentShader = compileShader(mGles2, GL_FRAGMENT_SHADER,
                                    kFrameTextureFragmentShader);

    // Links shaders to program.
    mProgram = mGles2->glCreateProgram();
    mGles2->glAttachShader(mProgram, mVertexShader);
    mGles2->glAttachShader(mProgram, mFragmentShader);
    mGles2->glLinkProgram(mProgram);
    mGles2->glUseProgram(mProgram);

    // Set shader attributes.
    GLint posAttrib = mGles2->glGetAttribLocation(mProgram, "in_position");
    mGles2->glEnableVertexAttribArray(posAttrib);
    mGles2->glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE,
                                  4 * sizeof(float), 0);

    GLint texcoordAttrib = mGles2->glGetAttribLocation(mProgram, "in_texcoord");
    mGles2->glEnableVertexAttribArray(texcoordAttrib);
    mGles2->glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE,
                                  4 * sizeof(float),
                                  (void*)(2 * sizeof(float)));


    mInitialized = true;
    return true;
}

void VideoplaybackRenderTarget::uninitialize() {
    if (!mInitialized) {
      return;
    }
    mGles2->glBindVertexArray(0);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mGles2->glBindTexture(GL_TEXTURE_2D, 0);
    mGles2->glDeleteVertexArrays(1, &mVao);
    mGles2->glDeleteTextures(1, &mTexture);
    mGles2->glDeleteBuffers(1, &mVertexBuffer);
    mGles2->glDeleteBuffers(1, &mElementBuffer);
    mGles2->glDetachShader(mProgram, mVertexShader);
    mGles2->glDetachShader(mProgram, mFragmentShader);
    mGles2->glDeleteShader(mVertexShader);
    mGles2->glDeleteShader(mFragmentShader);
    mGles2->glDeleteProgram(mProgram);
    mInitialized = false;
}

void VideoplaybackRenderTarget::renderBuffer() {
    if (!mInitialized) {
      LOG(ERROR) << "Videoplayback Render Target is not initialized.";
      return;
    }
    if (mBuffer == nullptr) {
        return;
    }

    const unsigned char* frameBuf = &mBuffer[mFrameInfo.headerlen];

    mGles2->glBindVertexArray(mVao);

    // Load the processed frame into the texture.
    if (mFrameInfo.width <= mRenderWidth &&
        mFrameInfo.height <= mRenderHeight) {
        mGles2->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mFrameInfo.width,
                                mFrameInfo.height, GL_RGB, GL_UNSIGNED_BYTE,
                                frameBuf);
    } else {
        // The frame dimensions were too big, so we should allocate a larger
        // texture.
        mGles2->glTexImage2D(GL_TEXTURE_2D, 0, 0, 0, mFrameInfo.width,
                             mFrameInfo.height, GL_RGB, GL_UNSIGNED_BYTE,
                             frameBuf);
        mRenderWidth = std::max(mRenderWidth, mFrameInfo.width);
        mRenderHeight = std::max(mRenderHeight, mFrameInfo.height);
    }

    mGles2->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

}  // namespace videoplayback
}  // namespace android
