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

#include "android/base/ArraySize.h"
#include "android/utils/debug.h"
#include "android/virtualscene/VirtualSceneCamera.h"
#include "android/virtualscene/VirtualSceneTexture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <cmath>

using namespace android::base;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

/*******************************************************************************
 *                     Renderer routines
 ******************************************************************************/

namespace android {
namespace virtualscene {

static constexpr char kSimpleVertexShader[] = R"(
attribute vec3 in_position;
attribute vec2 in_uv;

uniform mat4 u_modelViewProj;

varying vec2 uv;

void main() {
    uv = in_uv;
    gl_Position = u_modelViewProj * vec4(in_position, 1.0);
}
)";

static constexpr char kSimpleFragmentShader[] = R"(
precision mediump float;
varying vec2 uv;

uniform sampler2D tex_sampler;

void main() {
  gl_FragColor = texture2D(tex_sampler, uv);
}
)";

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

// clang-format off
// Vertices for a for a quad.
static constexpr Vertex kQuadVertices[] = {
    { glm::vec3(-5.0f,  5.0f, -5.0f), glm::vec2(0.0f, 0.0f) },  // top left
    { glm::vec3( 5.0f,  5.0f, -5.0f), glm::vec2(1.0f, 0.0f) },  // top right
    { glm::vec3( 5.0f, -5.0f, -5.0f), glm::vec2(1.0f, 1.0f) },  // bottom right
    { glm::vec3(-5.0f, -5.0f, -5.0f), glm::vec2(0.0f, 1.0f) },  // bottom left
};

static constexpr GLubyte kQuadIndices[] = {
  0, 2, 1,  // TL->BR->TR
  0, 3, 2,  // TL->BL->BR
};
// clang-format on

// The OpenGL coordinate system is right-handed with negative-Z forward.
//
// Cube quads are 10x10 meters, 5 meters away from the camera to form a 10-meter
// cube around the origin.
//
// The cube textures are oriented so that if you are looking at +z and look
// down, the -y texture is visible and oriented correctly, so rotate the quads
// relative to +z.
//
//       y  -z
//       | /
// -x ___|/____ x
//      /|
//     / |
//    z  -y

struct CubeSide {
    const char* texture;
    glm::quat rotation;
};

static constexpr float kPi = 3.14159265358979323846f;
static constexpr float kPi2 = kPi / 2.00f;

static const CubeSide kCubeSides[] = {
        {"micro_kitchen1_negz.png", glm::quat()},
        {"micro_kitchen1_posz.png",
         glm::angleAxis(kPi, glm::vec3(0.0f, 1.0f, 0.0f))},
        {"micro_kitchen1_posx.png",
         glm::angleAxis(kPi2, glm::vec3(0.0f, 1.0f, 0.0f))},
        {"micro_kitchen1_negx.png",
         glm::angleAxis(-kPi2, glm::vec3(0.0f, 1.0f, 0.0f))},
        {"micro_kitchen1_negy.png",
         glm::angleAxis(kPi, glm::vec3(0.0f, 1.0f, 0.0f)) *
                 glm::angleAxis(-kPi2, glm::vec3(1.0f, 0.0f, 0.0f))},
        {"micro_kitchen1_posy.png",
         glm::angleAxis(kPi, glm::vec3(0.0f, 1.0f, 0.0f)) *
                 glm::angleAxis(kPi2, glm::vec3(1.0f, 0.0f, 0.0f))},
};

VirtualSceneRenderer::VirtualSceneRenderer(const GLESv2Dispatch* gles2)
    : mGles2(gles2) {}

VirtualSceneRenderer::~VirtualSceneRenderer() {
    // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
    mGles2->glDeleteBuffers(1, &mVertexBuffer);
    mVertexBuffer = 0;

    mGles2->glDeleteBuffers(1, &mIndexBuffer);
    mIndexBuffer = 0;

    mGles2->glDeleteProgram(mProgram);
    mProgram = 0;
}

bool VirtualSceneRenderer::initialize() {
    for (const CubeSide& side : kCubeSides) {
        std::unique_ptr<VirtualSceneTexture> texture =
                VirtualSceneTexture::load(mGles2, side.texture);
        if (!texture) {
            return false;
        }

        mCubeTextures.push_back(std::move(texture));
    }

    // Compile and setup shaders.
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

    mPositionLocation = getAttribLocation(mProgram, "in_position");
    mUvLocation = getAttribLocation(mProgram, "in_uv");
    mTexSamplerLocation = getUniformLocation(mProgram, "tex_sampler");
    mMvpLocation = getUniformLocation(mProgram, "u_modelViewProj");

    mGles2->glGenBuffers(1, &mIndexBuffer);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    mGles2->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kQuadIndices),
                         kQuadIndices, GL_STATIC_DRAW);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mGles2->glGenBuffers(1, &mVertexBuffer);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    mGles2->glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices,
                         GL_STATIC_DRAW);

    // glVertexAttribPointer requires the GL_ARRAY_BUFFER to be bound, keep
    // it bound until all of the vertex attribs are initialized.
    if (mPositionLocation >= 0) {
        mGles2->glEnableVertexAttribArray(mPositionLocation);
        mGles2->glVertexAttribPointer(
                mPositionLocation,
                3,                                                // size
                GL_FLOAT,                                         // type
                GL_FALSE,                                         // normalized?
                sizeof(Vertex),                                   // stride
                reinterpret_cast<void*>(offsetof(Vertex, pos)));  // offset
        mGles2->glDisableVertexAttribArray(mPositionLocation);
    }
    if (mUvLocation >= 0) {
        mGles2->glEnableVertexAttribArray(mUvLocation);
        mGles2->glVertexAttribPointer(
                mUvLocation,
                2,                                               // size
                GL_FLOAT,                                        // type
                GL_FALSE,                                        // normalized?
                sizeof(Vertex),                                  // stride
                reinterpret_cast<void*>(offsetof(Vertex, uv)));  // offset
        mGles2->glDisableVertexAttribArray(mUvLocation);
    }

    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

std::unique_ptr<VirtualSceneRenderer> VirtualSceneRenderer::create(
        const GLESv2Dispatch* gles2) {
    std::unique_ptr<VirtualSceneRenderer> renderer;
    renderer.reset(new VirtualSceneRenderer(gles2));
    if (!renderer || !renderer->initialize()) {
        return nullptr;
    }

    return renderer;
}

GLuint VirtualSceneRenderer::compileShader(GLenum type,
                                           const char* shaderSource) {
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
            E("%s: Failed to compile shader: %s", __FUNCTION__, message.data());
        } else {
            E("%s: Failed to compile shader", __FUNCTION__);
        }

        mGles2->glDeleteShader(shaderId);
        return 0;
    }

    return shaderId;
}

GLuint VirtualSceneRenderer::linkShaders(GLuint vertexId, GLuint fragmentId) {
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
            E("%s: Failed to link program: %s", __FUNCTION__, message.data());
        } else {
            E("%s: Failed to link program", __FUNCTION__);
        }

        mGles2->glDeleteProgram(programId);
        return 0;
    }

    return programId;
}

GLint VirtualSceneRenderer::getAttribLocation(GLuint program,
                                              const char* name) {
    GLint location = mGles2->glGetAttribLocation(program, name);
    if (location < 0) {
        W("%s: Program attrib '%s' not found", __FUNCTION__, name);
    }

    return location;
}

GLint VirtualSceneRenderer::getUniformLocation(GLuint program,
                                               const char* name) {
    GLint location = mGles2->glGetUniformLocation(program, name);
    if (location < 0) {
        W("%s: Program uniform '%s' not found", __FUNCTION__, name);
    }

    return location;
}

void VirtualSceneRenderer::render() {
    mGles2->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT);

    // Transform view by rotation.
    mCamera.update();

    const glm::mat4 viewProj = mCamera.getViewProjection();

    mGles2->glUseProgram(mProgram);

    mGles2->glEnableVertexAttribArray(mPositionLocation);
    mGles2->glEnableVertexAttribArray(mUvLocation);

    mGles2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    mGles2->glActiveTexture(GL_TEXTURE0);
    mGles2->glUniform1i(mTexSamplerLocation, 0);

    // Build a 10-meter cube around the origin.
    for (size_t i = 0; i < arraySize(kCubeSides); ++i) {
        const glm::mat4 model = glm::toMat4(kCubeSides[i].rotation);

        const glm::mat4 mvp = viewProj * model;
        mGles2->glUniformMatrix4fv(mMvpLocation, 1, GL_FALSE, &mvp[0][0]);

        mGles2->glBindTexture(GL_TEXTURE_2D, mCubeTextures[i]->getTextureId());
        mGles2->glDrawElements(GL_TRIANGLES, arraySize(kQuadIndices),
                               GL_UNSIGNED_BYTE, nullptr);
    }

    mGles2->glUseProgram(0);
    mGles2->glDisableVertexAttribArray(mPositionLocation);
    mGles2->glDisableVertexAttribArray(mUvLocation);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mGles2->glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace virtualscene
}  // namespace android
