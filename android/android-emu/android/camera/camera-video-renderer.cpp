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

#include "android/camera/camera-video-renderer.h"

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Log.h"

namespace android {
namespace videoplayback {

static const char* kFrameTextureVertexShader = R"(
attribute vec2 in_position;
attribute vec2 in_texcoord;

varying vec2 texcoord;

void main() {
  texcoord = in_texcoord;
  gl_Position = vec4(in_position, 0.0, 1.0);
}
)";

static const char* kFrameTextureFragmentShader = R"(
precision mediump float;

varying vec2 texcoord;

vec4 v_rgb;
uniform sampler2D frameTexture;

void main() {
  v_rgb = texture2D(frameTexture, texcoord);
  gl_FragColor = v_rgb.zyxw;
}
)";

GLuint compileShader(const GLESv2Dispatch* const gles2,
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
    mRenderWidth = video_width;
    mRenderHeight = video_height;
    *resultRenderTargetWidth = mRenderWidth;
    *resultRenderTargetHeight = mRenderHeight;
}

void VideoplaybackRenderTarget::setPixelBuffer(const unsigned char* buf,
                                               size_t len) {
    mBuffer = buf;
    mBufferLen = len;
}

/* renderBuffer draws a rectangle to fill the viewport and textures
 * it with the provided video frame from the video player (stored in mBuffer).
 */
void VideoplaybackRenderTarget::renderBuffer() {
    GLuint vertexShader =
            compileShader(mGles2, GL_VERTEX_SHADER, kFrameTextureVertexShader);
    GLuint fragmentShader = compileShader(mGles2, GL_FRAGMENT_SHADER,
                                          kFrameTextureFragmentShader);

    GLuint shaderProgram = mGles2->glCreateProgram();
    mGles2->glAttachShader(shaderProgram, vertexShader);
    mGles2->glAttachShader(shaderProgram, fragmentShader);
    mGles2->glLinkProgram(shaderProgram);
    mGles2->glUseProgram(shaderProgram);

    GLfloat vertices[] = {
            //   Position  Tex Coords
            -1.0f,  1.0f,  -1.0f, 1.0f,  // Top-left
             1.0f,  1.0f,  1.0f, 1.0f,  // Top-right
             1.0f, -1.0f,  1.0f, -1.0f,  // Bottom-right
            -1.0f, -1.0f,  -1.0f, -1.0f,  // Bottom-left
    };

    // Set up
    GLuint elements[] = {0, 1, 2, 2, 3, 0};

    GLuint vao;
    mGles2->glGenVertexArrays(1, &vao);
    mGles2->glBindVertexArray(vao);

    mGles2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mGles2->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mGles2->glViewport(0, 0, mRenderWidth, mRenderHeight);

    if (mBuffer != nullptr) {
        // Create a rectangle for the viewport.
        GLuint vertexBuffer;
        mGles2->glGenBuffers(1, &vertexBuffer);
        mGles2->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        mGles2->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                             GL_STATIC_DRAW);

        GLuint elementBuffer;
        mGles2->glGenBuffers(1, &elementBuffer);
        mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
        mGles2->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements),
                             elements, GL_STATIC_DRAW);

        // Create a texture containing the video frame.
        GLuint texture;
        mGles2->glGenTextures(1, &texture);
        mGles2->glBindTexture(GL_TEXTURE_2D, texture);
        mGles2->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mRenderWidth,
                             mRenderHeight, 0, GL_RGB, GL_UNSIGNED_BYTE,
                             mBuffer);
        mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
        mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);

        // Set shader attributes
        GLint posAttrib =
                mGles2->glGetAttribLocation(shaderProgram, "in_position");
        mGles2->glEnableVertexAttribArray(posAttrib);
        mGles2->glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE,
                                      4 * sizeof(float), 0);

        GLint texcoordAttrib =
            mGles2->glGetAttribLocation(shaderProgram, "in_texcoord");
        mGles2->glEnableVertexAttribArray(texcoordAttrib);
        mGles2->glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE,
                                      4 * sizeof(float),
                                      (void*)(2 * sizeof(float)));

        // Draw the rectangle
        mGles2->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
}

// Base VideoRenderer functions
VideoRenderer::VideoRenderer() {}
VideoRenderer::~VideoRenderer() {}

bool VideoRenderer::initialize(const GLESv2Dispatch* gles2,
                               int width,
                               int height) {
    mRenderTarget = std::unique_ptr<VideoplaybackRenderTarget>(
            new VideoplaybackRenderTarget(gles2, width, height));
    return true;
}

void VideoRenderer::uninitialize() {
    mRenderTarget.reset();
}

int64_t VideoRenderer::render() {
    mRenderTarget->renderBuffer();
    return 0;
}

}  // namespace videoplayback
}  // namespace android
