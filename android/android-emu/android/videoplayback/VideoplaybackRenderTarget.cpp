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
#include "android/videoplayback/VideoplaybackUtils.h"

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
        height = static_cast<int>(width / sampleAspectRatio);
    }
    *resultRenderTargetWidth = width;
    *resultRenderTargetHeight = height;
}

void VideoplaybackRenderTarget::setPixelBuffer(const unsigned char* buf,
                                               size_t len) {
    mBuffer = buf;
    mBufferLen = len;
}

bool VideoplaybackRenderTarget::initialize() {
    // Vertices necessary to draw a rectangle (i.e. two triangles).
    GLfloat vertices[] = {
            // Position    Tex Coords
            -1.0f, 1.0f,  -1.0f, 1.0f,   // Top-left
            1.0f,  1.0f,  1.0f,  1.0f,   // Top-right
            1.0f,  -1.0f, 1.0f,  -1.0f,  // Bottom-right
            -1.0f, -1.0f, -1.0f, -1.0f,  // Bottom-left
    };

    GLuint elements[] = {0, 1, 2, 2, 3, 0};

    GLuint vao;
    mGles2->glGenVertexArrays(1, &vao);
    mGles2->glBindVertexArray(vao);

    mGles2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mGles2->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
}

void VideoplaybackRenderTarget::renderBuffer() {
    GLuint vertexShader =
            compileShader(mGles2, GL_VERTEX_SHADER, kFrameTextureVertexShader);
    GLuint fragmentShader = compileShader(mGles2, GL_FRAGMENT_SHADER,
                                          kFrameTextureFragmentShader);

    // Links shaders to program.
    GLuint program = mGles2->glCreateProgram();
    mGles2->glAttachShader(program, vertexShader);
    mGles2->glAttachShader(program, fragmentShader);
    mGles2->glLinkProgram(program);
    mGles2->glUseProgram(program);

    // Releases the shaders, because program will keep their reference alive.
    mGles2->glDeleteShader(vertexShader);
    mGles2->glDeleteShader(fragmentShader);

    // Somehow the viewport is only a fourth of the size of the drawn rectangle
    // when they use the same dimensions.
    mGles2->glViewport(0, 0, mRenderWidth * 2, mRenderHeight * 2);

    if (mBuffer != nullptr) {
        GLuint texture;
        PPMInfo frameInfo;
        base::Optional<size_t> headerSize = parsePPMHeader(&frameInfo, mBuffer, mBufferLen);
        if (headerSize) {
          const unsigned char* frameBuf = &mBuffer[*headerSize];

          // Load the processed frame into the texture.
          mGles2->glBindTexture(GL_TEXTURE_2D, mTexture);
          mGles2->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frameInfo.width,
                                  frameInfo.height, GL_RGB, GL_UNSIGNED_BYTE,
                                  frameBuf);

          // Set shader attributes.
          GLint posAttrib = mGles2->glGetAttribLocation(program, "in_position");
          mGles2->glEnableVertexAttribArray(posAttrib);
          mGles2->glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE,
                                        4 * sizeof(float), 0);

          GLint texcoordAttrib =
                  mGles2->glGetAttribLocation(program, "in_texcoord");
          mGles2->glEnableVertexAttribArray(texcoordAttrib);
          mGles2->glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE,
                                        4 * sizeof(float),
                                        (void*)(2 * sizeof(float)));

          mGles2->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }

    // Shader cleanup
    mGles2->glDetachShader(program, vertexShader);
    mGles2->glDetachShader(program, fragmentShader);
    mGles2->glDeleteProgram(program);
}

}  // namespace videoplayback
}  // namespace android
