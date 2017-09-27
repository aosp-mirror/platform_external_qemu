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

#include "android/virtualscene/VirtualSceneRenderer.h"

#include "android/utils/debug.h"
#include <vector>

using android::base::AutoLock;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(camera)

/*******************************************************************************
 *                     Renderer routines
 ******************************************************************************/

static constexpr char kSimpleVertexShader[] = R"(
attribute vec3 in_position;

void main() {
    gl_Position = vec4(in_position, 1.0);
}
)";

static constexpr char kSimpleFragmentShader[] = R"(
precision mediump float;

void main() {
  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

// clang-format off
// Vertices for a simple screen-space triangle.
static constexpr GLfloat kVertexBufferData[] = {
     1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
     0.0f, -1.0f, 0.0f,
};
// clang-format on

namespace android {
namespace virtualscene {

class VirtualSceneRendererImpl {
    DISALLOW_COPY_AND_ASSIGN(VirtualSceneRendererImpl);

public:
    VirtualSceneRendererImpl(const GLESv2Dispatch* gles2) : mGles2(gles2) {}

    bool initialize() {
        const GLuint vertexId =
                compileShader(GL_VERTEX_SHADER, kSimpleVertexShader);
        const GLuint fragmentId =
                compileShader(GL_FRAGMENT_SHADER, kSimpleFragmentShader);

        if (vertexId && fragmentId) {
            mProgram = linkShaders(vertexId, fragmentId);
        }

        // Release the shaders, the program will keep their reference alive.
        mGles2->glDeleteShader(vertexId);
        mGles2->glDeleteShader(fragmentId);

        if (!mProgram) {
            // Initialization failed, no program loaded.
            return false;
        }

        const GLint positionHandle =
                mGles2->glGetAttribLocation(mProgram, "in_position");

        if (positionHandle >= 0) {
            mGles2->glEnableVertexAttribArray(positionHandle);

            mGles2->glGenBuffers(1, &mVertexBuffer);
            mGles2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
            mGles2->glBufferData(GL_ARRAY_BUFFER, sizeof(kVertexBufferData),
                                 kVertexBufferData, GL_STATIC_DRAW);

            mGles2->glVertexAttribPointer(positionHandle,
                                          3,                     // size
                                          GL_FLOAT,              // type
                                          GL_FALSE,              // normalized?
                                          0,                     // stride
                                          static_cast<void*>(0)  // offset
            );
        }

        return true;
    }

    ~VirtualSceneRendererImpl() {
        // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
        mGles2->glDeleteBuffers(1, &mVertexBuffer);
        mVertexBuffer = 0;

        mGles2->glDeleteProgram(mProgram);
        mProgram = 0;
    }

    // Compile a shader from source.
    // |type| - GL shader type, such as GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
    // |shaderSource| - Shader source code.
    //
    // Returns a non-zero shader handle on success, or zero on failure.
    // The result must be released with glDeleteShader.
    GLuint compileShader(GLenum type, const char* shaderSource) {
        const GLuint shaderId = mGles2->glCreateShader(type);

        mGles2->glShaderSource(shaderId, 1, &shaderSource, nullptr);
        mGles2->glCompileShader(shaderId);

        // Output the error message if the compile failed.
        GLint result = GL_FALSE;
        mGles2->glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE) {
            int logLength = 0;
            mGles2->glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);

            if (logLength) {
                std::vector<char> message(logLength + 1);
                mGles2->glGetShaderInfoLog(shaderId, logLength, nullptr,
                                           message.data());
                E("%s: Failed to compile shader: %s", __FUNCTION__,
                  message.data());
            } else {
                E("%s: Failed to compile shader", __FUNCTION__);
            }

            mGles2->glDeleteShader(shaderId);
            return 0;
        }

        return shaderId;
    }

    // Link a vertex and fragment shader and return a program.
    // |vertexId| - GL vertex shader handle, must be non-zero.
    // |fragmentId| - GL fragment shader handle, must be non-zero.
    //
    // After this call, it is valid to call glDeleteShader; if the program was
    // successfully linked the program will keep them alive.
    //
    // Returns a non-zero program handle on success, or zero on failure.
    // The result must be released with glDeleteProgram.
    GLuint linkShaders(GLuint vertexId, GLuint fragmentId) {
        const GLuint programId = mGles2->glCreateProgram();
        mGles2->glAttachShader(programId, vertexId);
        mGles2->glAttachShader(programId, fragmentId);
        mGles2->glLinkProgram(programId);

        // Output the error message if the program failed to link.
        GLint result = GL_FALSE;
        mGles2->glGetProgramiv(programId, GL_LINK_STATUS, &result);
        if (result != GL_TRUE) {
            int logLength = 0;
            mGles2->glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

            if (logLength) {
                std::vector<char> message(logLength + 1);
                mGles2->glGetProgramInfoLog(programId, logLength, nullptr,
                                            message.data());
                E("%s: Failed to link program: %s", __FUNCTION__,
                  message.data());
            } else {
                E("%s: Failed to link program", __FUNCTION__);
            }

            mGles2->glDeleteProgram(programId);
            return 0;
        }

        return programId;
    }

    void render() {
        mBackgroundColor += 0.02f;
        if (mBackgroundColor > 1.0f) {
            mBackgroundColor = 0.2f;
        }

        mGles2->glClearColor(mBackgroundColor, 0.0f, 0.0f, 1.0f);
        mGles2->glClear(GL_COLOR_BUFFER_BIT);

        mGles2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

        // Draw the triangle!
        mGles2->glUseProgram(mProgram);
        mGles2->glDrawArrays(
                GL_TRIANGLES, 0,
                3);  // Starting from vertex 0; 3 vertices total -> 1 triangle

        mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

private:
    const GLESv2Dispatch* const mGles2;

    GLuint mProgram = 0;
    GLuint mVertexBuffer = 0;

    float mBackgroundColor = 0.2f;
};

/*******************************************************************************
 *                     Renderer wrapper
 ******************************************************************************/

android::base::LazyInstance<android::base::Lock> VirtualSceneRenderer::mLock =
        LAZY_INSTANCE_INIT;
VirtualSceneRendererImpl* VirtualSceneRenderer::mImpl = nullptr;

bool VirtualSceneRenderer::initialize(const GLESv2Dispatch* gles2) {
    AutoLock lock(mLock.get());
    if (mImpl) {
        E("VirtualSceneRenderer already initialized");
        return false;
    }

    mImpl = new VirtualSceneRendererImpl(gles2);
    if (!mImpl) {
        E("VirtualSceneRenderer failed to construct");
        return false;
    }

    const bool result = mImpl->initialize();
    if (!result) {
        delete mImpl;
        mImpl = nullptr;
    }

    return result;
}

void VirtualSceneRenderer::uninitialize() {
    AutoLock lock(mLock.get());
    if (!mImpl) {
        E("VirtualSceneRenderer not initialized");
        return;
    }

    delete mImpl;
    mImpl = nullptr;
}

void VirtualSceneRenderer::render() {
    AutoLock lock(mLock.get());
    if (!mImpl) {
        E("VirtualSceneRenderer not initialized");
        return;
    }

    mImpl->render();
}

}  // namespace virtualscene
}  // namespace android
