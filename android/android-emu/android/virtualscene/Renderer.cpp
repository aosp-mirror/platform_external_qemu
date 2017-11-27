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

Renderer::Renderer(const GLESv2Dispatch* gles2) : mGles2(gles2) {}

Renderer::~Renderer() = default;

bool Renderer::initialize() {
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

    return true;
}

std::unique_ptr<Renderer> Renderer::create(const GLESv2Dispatch* gles2) {
    std::unique_ptr<Renderer> renderer;
    renderer.reset(new Renderer(gles2));
    if (!renderer || !renderer->initialize()) {
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
