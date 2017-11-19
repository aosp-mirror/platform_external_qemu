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

#pragma once

/*
 * Defines the Virtual Scene Renderer, used by the Virtual Scene Camera
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/utils/compiler.h"
#include "android/virtualscene/SceneCamera.h"
#include "android/virtualscene/SceneObject.h"
#include "android/virtualscene/Texture.h"

#include <vector>

namespace android {
namespace virtualscene {

class Renderer {
    DISALLOW_COPY_AND_ASSIGN(Renderer);

public:
    ~Renderer();

    // Create a virtual scene Renderer.
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    //
    // Returns a Renderer instance if the renderer was successfully created or
    // null if there was an error.
    static std::unique_ptr<Renderer> create(const GLESv2Dispatch* gles2);

    const SceneCamera& getCamera() const;
    const GLESv2Dispatch* getGLESv2Dispatch();

    // Compile a shader from source.
    // |type| - GL shader type, such as GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
    // |shaderSource| - Shader source code.
    //
    // Returns a non-zero shader handle on success, or zero on failure.
    // The result must be released with glDeleteShader.
    GLuint compileShader(GLenum type, const char* shaderSource);

    // Link a vertex and fragment shader and return a program.
    // |vertexId| - GL vertex shader handle, must be non-zero.
    // |fragmentId| - GL fragment shader handle, must be non-zero.
    //
    // After this call, it is valid to call glDeleteShader; if the program was
    // successfully linked the program will keep them alive.
    //
    // Returns a non-zero program handle on success, or zero on failure.
    // The result must be released with glDeleteProgram.
    GLuint linkShaders(GLuint vertexId, GLuint fragmentId);

    GLint getAttribLocation(GLuint program, const char* name);
    GLint getUniformLocation(GLuint program, const char* name);

    void render();

private:
    // Private constructor, use Renderer::create to create an
    // instance.
    Renderer(const GLESv2Dispatch* gles2);

    // Initial helper to initialize the Renderer.
    bool initialize();

    // Helper to create a SceneObject from an obj file and texture.
    // |objFile| - Filename of the obj file to load.
    // |textureFile| - Filename of the texture to load.
    //
    // Returns the SceneObject is loading was successful, or null otherwise.
    std::unique_ptr<SceneObject> loadSceneObject(const char* objFile,
                                                 const char* textureFile);

    const GLESv2Dispatch* const mGles2;

    SceneCamera mCamera;

    std::vector<std::unique_ptr<SceneObject>> mSceneObjects;
};

}  // namespace virtualscene
}  // namespace android
