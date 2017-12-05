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

#include "android/virtualscene/SceneObject.h"

#include "android/utils/debug.h"
#include "android/virtualscene/Renderer.h"
#include "android/virtualscene/Texture.h"

#define W(...) dwarning(__VA_ARGS__)

namespace android {
namespace virtualscene {

static constexpr char kScreenSpaceVertexShader[] = R"(
attribute vec2 in_pos;
attribute vec2 in_uv;

varying vec2 uv;

void main() {
    uv = in_uv;
    gl_Position = vec4(in_pos, 0.0, 1.0);
}
)";

static constexpr GLfloat kScreenQuadVerts[] = {
    -1.f, -1.f,  0.f,  0.f,
     3.f, -1.f,  2.f,  0.f,
    -1.f,  3.f,  0.f,  2.f,
};

static constexpr GLubyte kScreenQuadIndices[] = {
    0, 1, 2,
};

Effect::Effect(Renderer& renderer)
    : mRenderer(renderer) {}

Effect::~Effect() {
    const GLESv2Dispatch* gles2 = mRenderer.getGLESv2Dispatch();

    if (gles2) {
        // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
        gles2->glDeleteBuffers(1, &mVertexBuffer);
        gles2->glDeleteBuffers(1, &mIndexBuffer);
        gles2->glDeleteProgram(mProgram);
    }
}

bool Effect::initialize(const char* frag) {
    const GLESv2Dispatch* gles2 = mRenderer.getGLESv2Dispatch();

    // Compile and setup shaders.
    const GLuint vertexId =
            mRenderer.compileShader(GL_VERTEX_SHADER, kScreenSpaceVertexShader);
    const GLuint fragmentId =
            mRenderer.compileShader(GL_FRAGMENT_SHADER, frag);

    if (vertexId && fragmentId) {
        mProgram = mRenderer.linkShaders(vertexId, fragmentId);
    }

    // Release the shaders, the program will keep their reference alive.
    gles2->glDeleteShader(vertexId);
    gles2->glDeleteShader(fragmentId);

    if (!mProgram) {
        // Initialization failed, no program loaded.
        return false;
    }

    mPositionLocation = mRenderer.getAttribLocation(mProgram, "in_pos");
    mUvLocation = mRenderer.getAttribLocation(mProgram, "in_uv");
    mTextureSamplerLocation = mRenderer.getUniformLocation(mProgram,
                                                           "tex_sampler");
    mResolutionLocation = mRenderer.getUniformLocation(mProgram,
                                                       "resolution");

    gles2->glGenBuffers(1, &mVertexBuffer);
    gles2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    gles2->glBufferData(GL_ARRAY_BUFFER, sizeof(kScreenQuadVerts),
                         kScreenQuadVerts, GL_STATIC_DRAW);
    gles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    gles2->glGenBuffers(1, &mIndexBuffer);
    gles2->glBindBuffer(GL_ARRAY_BUFFER, mIndexBuffer);
    gles2->glBufferData(GL_ARRAY_BUFFER, sizeof(kScreenQuadIndices),
                         kScreenQuadIndices, GL_STATIC_DRAW);
    gles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

std::unique_ptr<Effect> Effect::createEffect(Renderer& renderer,
                                             const char* frag) {
    std::unique_ptr<Effect> result(new Effect(renderer));
    if (!result->initialize(frag)) {
        return nullptr;
    }

    return result;
}

void Effect::render(const Texture* inputTexture, int width, int height) {
    const GLESv2Dispatch* gles2 = mRenderer.getGLESv2Dispatch();

    gles2->glUseProgram(mProgram);

    if (mResolutionLocation >= 0) {
        gles2->glUniform2f(mResolutionLocation, 1.f / static_cast<float>(width),
                           1.f / static_cast<float>(height));
    }

    if (mTextureSamplerLocation >= 0 && inputTexture != nullptr) {
        gles2->glActiveTexture(GL_TEXTURE0);
        gles2->glUniform1i(mTextureSamplerLocation, 0);
        gles2->glBindTexture(GL_TEXTURE_2D,
                             inputTexture->getTextureId());
    }

    gles2->glEnableVertexAttribArray(mPositionLocation);
    gles2->glEnableVertexAttribArray(mUvLocation);

    gles2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    gles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

    gles2->glVertexAttribPointer(mPositionLocation, 2, GL_FLOAT, GL_FALSE,
                                  4 * sizeof(GLfloat), 0);
    gles2->glVertexAttribPointer(mUvLocation, 2, GL_FLOAT, GL_FALSE,
                                  4 * sizeof(GLfloat),
                                  reinterpret_cast<void*>(2 * sizeof(GLfloat)));

    gles2->glDrawElements(GL_TRIANGLES,
                           sizeof(kScreenQuadIndices) / sizeof(GLubyte),
                           GL_UNSIGNED_BYTE, nullptr);

    gles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    gles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    gles2->glDisableVertexAttribArray(mUvLocation);
    gles2->glDisableVertexAttribArray(mPositionLocation);

    gles2->glBindTexture(GL_TEXTURE_2D, 0);

    gles2->glUseProgram(0);
}

}  // namespace virtualscene
}  // namespace android
