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

static constexpr char kScreenSpaceVertexShader[] = R"(
attribute vec2 in_pos;
attribute vec2 in_uv;

varying vec2 uv;

void main() {
    uv = in_uv;
    gl_Position = vec4(in_pos, 0.0, 1.0);
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

static constexpr GLfloat kScreenQuadVerts[] = {
    -1.f, -1.f,  0.f,  0.f,
     1.f, -1.f,  1.f,  0.f,
     1.f,  1.f,  1.f,  1.f,
    -1.f,  1.f,  0.f,  1.f,
};

static constexpr GLubyte kScreenQuadIndices[] = {
    0, 1, 2,
    0, 2, 3,
};

Renderer::Renderer(const GLESv2Dispatch* gles2) : mGles2(gles2) {}

Renderer::~Renderer() {
    mGles2->glDeleteTextures(1, &mRenderTexture);
    mGles2->glDeleteFramebuffers(1, &mRenderFramebuffer);
    mGles2->glDeleteRenderbuffers(1, &mRenderbufferDepth);
}

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

    // Compile and setup shaders.
    const GLuint vert = compileShader(GL_VERTEX_SHADER,
                                      kScreenSpaceVertexShader);
    const GLuint frag = compileShader(GL_FRAGMENT_SHADER,
                                      kSimpleFragmentShader);

    if (vert && frag) {
        mScreenQuadProgram = linkShaders(vert, frag);
    }

    mGles2->glDeleteShader(vert);
    mGles2->glDeleteShader(frag);

    if (!mScreenQuadProgram) {
        return false;
    }

    mPosLocation = getAttribLocation(mScreenQuadProgram, "in_pos");
    mUvLocation = getAttribLocation(mScreenQuadProgram, "in_uv");
    mTexSampler = getUniformLocation(mScreenQuadProgram, "tex_sampler");

    mGles2->glGenFramebuffers(1, &mRenderFramebuffer);
    mGles2->glBindFramebuffer(GL_FRAMEBUFFER, mRenderFramebuffer);

    mGles2->glGenTextures(1, &mRenderTexture);
    mGles2->glBindTexture(GL_TEXTURE_2D, mRenderTexture);
    mGles2->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, 0);

    mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    mGles2->glGenRenderbuffers(1, &mRenderbufferDepth);
    mGles2->glBindRenderbuffer(GL_RENDERBUFFER, mRenderbufferDepth);
    mGles2->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                                  width, height);
    mGles2->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER, mRenderbufferDepth);

    mGles2->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, mRenderTexture, 0);

    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);

    if (mGles2->glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }

    mGles2->glGenBuffers(1, &mScreenQuadVerts);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, mScreenQuadVerts);
    mGles2->glBufferData(GL_ARRAY_BUFFER, sizeof(kScreenQuadVerts),
                         kScreenQuadVerts, GL_STATIC_DRAW);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    mGles2->glGenBuffers(1, &mScreenQuadIndices);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, mScreenQuadIndices);
    mGles2->glBufferData(GL_ARRAY_BUFFER, sizeof(kScreenQuadIndices),
                         kScreenQuadIndices, GL_STATIC_DRAW);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    mRenderWidth = width;
    mRenderHeight = height;

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

int64_t Renderer::render(int width, int height) {
    mGles2->glBindFramebuffer(GL_FRAMEBUFFER, mRenderFramebuffer);
    mGles2->glViewport(0, 0, mRenderWidth, mRenderHeight);
    mGles2->glEnable(GL_DEPTH_TEST);
    mGles2->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Transform view by rotation.
    int64_t timestamp = mCamera.update();

    // Render scene objects.
    for (auto& sceneObject : mSceneObjects) {
        sceneObject->render();
    }

    mGles2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mGles2->glViewport(0, 0, width, height);
    mGles2->glDisable(GL_DEPTH_TEST);

    mGles2->glUseProgram(mScreenQuadProgram);

    mGles2->glActiveTexture(GL_TEXTURE0);
    mGles2->glUniform1i(mTexSampler, 0);
    mGles2->glBindTexture(GL_TEXTURE_2D, mRenderTexture);

    mGles2->glEnableVertexAttribArray(mPosLocation);
    mGles2->glEnableVertexAttribArray(mUvLocation);

    mGles2->glBindBuffer(GL_ARRAY_BUFFER, mScreenQuadVerts);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mScreenQuadIndices);

    mGles2->glVertexAttribPointer(mPosLocation, 2, GL_FLOAT, GL_FALSE,
                                  4 * sizeof(GLfloat), 0);
    mGles2->glVertexAttribPointer(mUvLocation, 2, GL_FLOAT, GL_FALSE,
                                  4 * sizeof(GLfloat),
                                  reinterpret_cast<void*>(2 * sizeof(GLfloat)));

    mGles2->glDrawElements(GL_TRIANGLES,
                           sizeof(kScreenQuadIndices) / sizeof(GLubyte),
                           GL_UNSIGNED_BYTE, nullptr);

    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    mGles2->glDisableVertexAttribArray(mUvLocation);
    mGles2->glDisableVertexAttribArray(mPosLocation);

    mGles2->glBindTexture(GL_TEXTURE_2D, 0);
    mGles2->glUseProgram(0);

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
