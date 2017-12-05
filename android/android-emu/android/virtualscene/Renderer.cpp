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

#include "android/virtualscene/Renderer.h"

#include "android/base/ArraySize.h"
#include "android/utils/debug.h"
#include "android/virtualscene/Effect.h"
#include "android/virtualscene/SceneObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>

using namespace android::base;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

/*******************************************************************************
 *                     Renderer routines
 ******************************************************************************/

static constexpr char kTexturedVertexShader[] = R"(
attribute vec3 in_position;
attribute vec2 in_uv;

uniform mat4 u_modelViewProj;

varying vec2 uv;

void main() {
    uv = in_uv;
    gl_Position = u_modelViewProj * vec4(in_position, 1.0);
}
)";

static constexpr char kTexturedFragmentShader[] = R"(
precision mediump float;
varying vec2 uv;

uniform sampler2D tex_sampler;

void main() {
  gl_FragColor = texture2D(tex_sampler, uv);
}
)";

static constexpr char kScreenSpaceVertexShader[] = R"(
attribute vec3 in_position;
attribute vec2 in_uv;

varying vec2 uv;

void main() {
    uv = in_uv;
    gl_Position = vec4(in_position, 1.0);
}
)";

static constexpr char kBlitFragmentShader[] = R"(
precision mediump float;
uniform vec2 resolution;

uniform sampler2D tex_sampler;

void main() {
  gl_FragColor = texture2D(tex_sampler, gl_FragCoord.xy * resolution);
}
)";

static constexpr char kFxaaFragmentShader[] = R"(
precision mediump float;
uniform sampler2D tex_sampler;
uniform vec2 resolution;

#define FXAA_MUL  (1.0/8.0)
#define FXAA_MIN  (1.0/32.0)
#define FXAA_SPAN 8.0

void main() {
  vec3 rgbNW = texture2D(tex_sampler, (gl_FragCoord.xy + vec2(-1.0, -1.0)) * resolution).xyz;
  vec3 rgbNE = texture2D(tex_sampler, (gl_FragCoord.xy + vec2( 1.0, -1.0)) * resolution).xyz;
  vec3 rgbM = texture2D(tex_sampler, gl_FragCoord.xy * resolution).xyz;
  vec3 rgbSW = texture2D(tex_sampler, (gl_FragCoord.xy + vec2(-1.0,  1.0)) * resolution).xyz;
  vec3 rgbSE = texture2D(tex_sampler, (gl_FragCoord.xy + vec2( 1.0,  1.0)) * resolution).xyz;

  vec3 luma = vec3(0.299, 0.587, 0.114);
  float lumaNW = dot(rgbNW, luma);
  float lumaNE = dot(rgbNE, luma);
  float lumaM = dot(rgbM, luma);
  float lumaSW = dot(rgbSW, luma);
  float lumaSE = dot(rgbSE, luma);

  vec2 dir = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)),
                  ((lumaNW + lumaSW) - (lumaNE + lumaSE)));

  float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_MUL), FXAA_MIN);

  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

  float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

  dir = min(vec2( FXAA_SPAN, FXAA_SPAN), max(vec2(-FXAA_SPAN, -FXAA_SPAN), dir * rcpDirMin)) * resolution;

  vec4 rgbA = 0.5 * (texture2D(tex_sampler, gl_FragCoord.xy * resolution + dir * (1.0 / 3.0 - 0.5)) +
                     texture2D(tex_sampler, gl_FragCoord.xy * resolution + dir * (2.0 / 3.0 - 0.5)));
  vec4 rgbB = rgbA * 0.5 + 0.25 * (texture2D(tex_sampler, gl_FragCoord.xy * resolution + dir * (0.0 / 3.0 - 0.5)) +
                                   texture2D(tex_sampler, gl_FragCoord.xy * resolution + dir * (3.0 / 3.0 - 0.5)));
  float lumaB = dot(rgbB.xyz, luma);

  if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
      gl_FragColor = rgbA;
  } else {
      gl_FragColor = rgbB;
  }
}
)";

namespace android {
namespace virtualscene {

static constexpr int kSuperSampleMultiple = 2;

Renderer::Renderer(const GLESv2Dispatch* gles2) : mGles2(gles2) {}

Renderer::~Renderer() = default;

bool Renderer::initialize(int width, int height) {
    mRenderWidth = width;
    mRenderHeight = height;

    mScreenRenderTarget = RenderTarget::createDefault(mGles2, width, height);
    mRenderTargets[0] = RenderTarget::createTextureTarget(mGles2,
            width * kSuperSampleMultiple, height * kSuperSampleMultiple);
    mRenderTargets[1] = RenderTarget::createTextureTarget(mGles2,
            width * kSuperSampleMultiple, height * kSuperSampleMultiple);

    std::unique_ptr<Effect> fxaaEffect = Effect::createEffect(*this,
            kFxaaFragmentShader);
    mEffectsChain.push_back(std::move(fxaaEffect));
    std::unique_ptr<Effect> blitEffect =
            Effect::createEffect(*this, kBlitFragmentShader);
    mEffectsChain.push_back(std::move(blitEffect));

    std::unique_ptr<SceneObject> main = loadSceneObject("Toren1BD_Main.obj");
    if (!main) {
        return false;
    }
    mSceneObjects.push_back(std::move(main));

    std::unique_ptr<SceneObject> decor = loadSceneObject("Toren1BD_Decor.obj");
    if (!decor) {
        return false;
    }
    mSceneObjects.push_back(std::move(decor));

    std::unique_ptr<SceneObject> tv = loadSceneObject("Toren1BD_TVScreen.obj");
    if (!tv) {
        return false;
    }
    mSceneObjects.push_back(std::move(tv));

    return true;
}

std::unique_ptr<Renderer> Renderer::create(const GLESv2Dispatch* gles2,
                                           int width, int height) {
    std::unique_ptr<Renderer> renderer;
    renderer.reset(new Renderer(gles2));
    if (!renderer || !renderer->initialize(width, height)) {
        return nullptr;
    }

    return renderer;
}

const SceneCamera& Renderer::getCamera() const {
    return mCamera;
}

const GLESv2Dispatch* Renderer::getGLESv2Dispatch() {
    return mGles2;
}

std::shared_ptr<Material> Renderer::createMaterialTextured() {
    if (mMaterialTextured) {
        return mMaterialTextured;
    }

    // Compile and setup shaders.
    const GLuint vertexId =
            compileShader(GL_VERTEX_SHADER, kTexturedVertexShader);
    const GLuint fragmentId =
            compileShader(GL_FRAGMENT_SHADER, kTexturedFragmentShader);

    GLuint program = 0;
    if (vertexId && fragmentId) {
        program = linkShaders(vertexId, fragmentId);
    }

    // Release the shaders, the program will keep their reference alive.
    mGles2->glDeleteShader(vertexId);
    mGles2->glDeleteShader(fragmentId);

    if (!program) {
        // Initialization failed, no program loaded.
        return nullptr;
    }

    std::shared_ptr<Material> material;
    material.reset(new Material(mGles2, program));

    material->mPositionLocation = getAttribLocation(program, "in_position");
    material->mUvLocation = getAttribLocation(program, "in_uv");
    material->mTexSamplerLocation = getUniformLocation(program, "tex_sampler");
    material->mMvpLocation = getUniformLocation(program, "u_modelViewProj");

    mMaterialTextured = material;
    return material;
}

std::shared_ptr<Material> Renderer::createMaterialScreenSpace(
        const char* frag) {
    // Compile and setup shaders.
    const GLuint vertexId =
            compileShader(GL_VERTEX_SHADER, kScreenSpaceVertexShader);
    const GLuint fragmentId = compileShader(GL_FRAGMENT_SHADER, frag);

    GLuint program = 0;
    if (vertexId && fragmentId) {
        program = linkShaders(vertexId, fragmentId);
    }

    // Release the shaders, the program will keep their reference alive.
    mGles2->glDeleteShader(vertexId);
    mGles2->glDeleteShader(fragmentId);

    if (!program) {
        // Initialization failed, no program loaded.
        return nullptr;
    }

    std::shared_ptr<Material> material;
    material.reset(new Material(mGles2, program));

    material->mPositionLocation = getAttribLocation(program, "in_position");
    material->mUvLocation = getAttribLocation(program, "in_uv");
    material->mTexSamplerLocation = getUniformLocation(program, "tex_sampler");
    material->mResolutionLocation = getUniformLocation(program, "resolution");

    return material;
}

std::shared_ptr<Mesh> Renderer::createMesh(const VertexPositionUV* vertices,
                                           size_t verticesSize,
                                           const GLuint* indices,
                                           size_t indicesSize) {
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;

    mGles2->glGenBuffers(1, &vertexBuffer);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    mGles2->glBufferData(GL_ARRAY_BUFFER,
                         verticesSize * sizeof(VertexPositionUV), vertices,
                         GL_STATIC_DRAW);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    mGles2->glGenBuffers(1, &indexBuffer);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    mGles2->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize * sizeof(GLuint),
                         indices, GL_STATIC_DRAW);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    VertexInfo info;
    info.hasUV = true;
    info.vertexSize = sizeof(VertexPositionUV);
    info.posOffset = offsetof(VertexPositionUV, pos);
    info.uvOffset = offsetof(VertexPositionUV, uv);

    std::shared_ptr<Mesh> mesh;
    mesh.reset(new Mesh(mGles2, info, vertexBuffer, indexBuffer, indicesSize));

    return mesh;
}

int64_t Renderer::render() {
    mRenderTargets[0]->bind();

    mGles2->glEnable(GL_DEPTH_TEST);
    mGles2->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Transform view by rotation.
    int64_t timestamp = mCamera.update();

    // Render scene objects.
    for (auto& sceneObject : mSceneObjects) {
        const glm::mat4 mvp =
                getCamera().getViewProjection() * sceneObject->getTransform();

        for (const RenderCommand& renderCommand :
             sceneObject->getRenderCommands()) {
            renderCommandExecute(
                    renderCommand,
                    [&](const std::shared_ptr<Material>& material) {
                        mGles2->glUniformMatrix4fv(material->mMvpLocation, 1,
                                                   GL_FALSE, &mvp[0][0]);
                    });
        }
    }

    mGles2->glDisable(GL_DEPTH_TEST);

    assert(!mEffectsChain.empty());
    for (size_t i = 0; i < mEffectsChain.size(); i++) {
        int superSampleMultiple = kSuperSampleMultiple;
        if (i == mEffectsChain.size() - 1) {
            superSampleMultiple = 1;
            mScreenRenderTarget->bind();
        } else {
            mRenderTargets[(i + 1) % 2]->bind();
        }

        RenderCommand renderCommand = mEffectsChain[i]->getRenderCommand(
                mRenderTargets[i % 2]->getTexture());
        renderCommandExecute(
                renderCommand, [&](const std::shared_ptr<Material>& material) {
                    mGles2->glUniform2f(
                            material->mResolutionLocation,
                            1.f / (superSampleMultiple * mRenderWidth),
                            1.f / (superSampleMultiple * mRenderHeight));
                });
    }

    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mGles2->glBindTexture(GL_TEXTURE_2D, 0);
    mGles2->glUseProgram(0);

    return timestamp;
}

std::unique_ptr<SceneObject> Renderer::loadSceneObject(const char* objFile) {
    std::unique_ptr<SceneObject> obj = SceneObject::loadFromObj(*this, objFile);
    if (!obj) {
        return nullptr;
    }

    return obj;
}

GLuint Renderer::compileShader(GLenum type, const char* shaderSource) {
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

GLuint Renderer::linkShaders(GLuint vertexId, GLuint fragmentId) {
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

GLint Renderer::getAttribLocation(GLuint program, const char* name) {
    GLint location = mGles2->glGetAttribLocation(program, name);
    if (location < 0) {
        W("%s: Program attrib '%s' not found", __FUNCTION__, name);
    }

    return location;
}

GLint Renderer::getUniformLocation(GLuint program, const char* name) {
    GLint location = mGles2->glGetUniformLocation(program, name);
    if (location < 0) {
        W("%s: Program uniform '%s' not found", __FUNCTION__, name);
    }

    return location;
}

void Renderer::renderCommandExecute(
        const RenderCommand& renderCommand,
        RenderCommandParameterCallback parameterCallback) {
    if (!renderCommand.material || !renderCommand.mesh) {
        return;
    }

    const std::shared_ptr<Material>& material = renderCommand.material;
    const VertexInfo& vertexInfo = renderCommand.mesh->mVertexInfo;

    mGles2->glUseProgram(material->mProgram);

    // Bind the texture if it exists.
    if (renderCommand.texture) {
        mGles2->glActiveTexture(GL_TEXTURE0);
        mGles2->glUniform1i(material->mTexSamplerLocation, 0);

        mGles2->glBindTexture(GL_TEXTURE_2D,
                              renderCommand.texture->getTextureId());
    } else {
        mGles2->glBindTexture(GL_TEXTURE_2D, 0);
    }

    parameterCallback(renderCommand.material);

    mGles2->glBindBuffer(GL_ARRAY_BUFFER, renderCommand.mesh->mVertexBuffer);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                         renderCommand.mesh->mIndexBuffer);

    // glVertexAttribPointer requires the GL_ARRAY_BUFFER to be bound, keep
    // it bound until all of the vertex attribs are initialized.
    if (material->mPositionLocation >= 0) {
        mGles2->glEnableVertexAttribArray(material->mPositionLocation);
        mGles2->glVertexAttribPointer(
                material->mPositionLocation,
                3,                                               // size
                GL_FLOAT,                                        // type
                GL_FALSE,                                        // normalized?
                vertexInfo.vertexSize,                           // stride
                reinterpret_cast<void*>(vertexInfo.posOffset));  // offset
    }

    if (material->mUvLocation >= 0 && vertexInfo.hasUV) {
        mGles2->glEnableVertexAttribArray(material->mUvLocation);
        mGles2->glVertexAttribPointer(
                material->mUvLocation,
                2,                                              // size
                GL_FLOAT,                                       // type
                GL_FALSE,                                       // normalized?
                vertexInfo.vertexSize,                          // stride
                reinterpret_cast<void*>(vertexInfo.uvOffset));  // offset
    }

    mGles2->glDrawElements(GL_TRIANGLES, renderCommand.mesh->mIndexCount,
                           GL_UNSIGNED_INT, nullptr);

    if (material->mPositionLocation >= 0) {
        mGles2->glDisableVertexAttribArray(material->mPositionLocation);
    }
    if (material->mUvLocation >= 0 && vertexInfo.hasUV) {
        mGles2->glDisableVertexAttribArray(material->mUvLocation);
    }
}

}  // namespace virtualscene
}  // namespace android
