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

namespace android {
namespace virtualscene {

static constexpr char kSimpleFragmentShader[] = R"(
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

static constexpr int kSuperSampleMultiple = 2;

Renderer::Renderer(const GLESv2Dispatch* gles2) : mGles2(gles2) {}

Renderer::~Renderer() = default;

bool Renderer::initialize(int width, int height) {
    const glm::mat4 translation =
            glm::translate(glm::mat4(), glm::vec3(0.0f, -1.0f, 0.0f));

    std::unique_ptr<SceneObject> main = loadSceneObject(
            "Toren1BD_Main.obj", "Toren1BD_Main_BakedLighting.png");
    if (!main) {
        return false;
    }

    main->setTransform(translation);
    mSceneObjects.push_back(std::move(main));

    std::unique_ptr<SceneObject> decor = loadSceneObject(
            "Toren1BD_Decor.obj", "Toren1BD_Decor_BakedLighting.png");
    if (!decor) {
        return false;
    }

    decor->setTransform(translation);
    mSceneObjects.push_back(std::move(decor));

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
    std::unique_ptr<Effect> blitEffect = Effect::createEffect(*this,
            kSimpleFragmentShader);
    mEffectsChain.push_back(std::move(blitEffect));

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

int64_t Renderer::render() {
    mRenderTargets[0]->bind();

    mGles2->glEnable(GL_DEPTH_TEST);
    mGles2->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Transform view by rotation.
    int64_t timestamp = mCamera.update();

    // Render scene objects.
    for (auto& sceneObject : mSceneObjects) {
        sceneObject->render();
    }

    mGles2->glDisable(GL_DEPTH_TEST);

    assert(mEffectsChain.size() > 0);
    for (int i = 0; i < mEffectsChain.size(); i++) {
        int superSampleMultiple = kSuperSampleMultiple;
        if (i == mEffectsChain.size() - 1) {
            superSampleMultiple = 1;
            mScreenRenderTarget->bind();
        } else {
            mRenderTargets[(i + 1) % 2]->bind();
        }

        mEffectsChain[i]->render(mRenderTargets[i % 2]->getTexture(),
                                 superSampleMultiple * mRenderWidth,
                                 superSampleMultiple * mRenderHeight);
    }

    return timestamp;
}

std::unique_ptr<SceneObject> Renderer::loadSceneObject(
        const char* objFile,
        const char* textureFile) {
    std::unique_ptr<SceneObject> obj = SceneObject::loadFromObj(*this, objFile);
    if (!obj) {
        return nullptr;
    }

    std::unique_ptr<Texture> texture = Texture::load(mGles2, textureFile);
    if (!texture) {
        return nullptr;
    }

    obj->setTexture(std::move(texture));
    return obj;
}

}  // namespace virtualscene
}  // namespace android
