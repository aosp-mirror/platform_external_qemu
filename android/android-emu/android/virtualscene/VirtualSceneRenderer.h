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
#include "android/virtualscene/VirtualSceneCamera.h"
#include "android/virtualscene/VirtualSceneTexture.h"

namespace android {
namespace virtualscene {

class VirtualSceneRenderer {
    DISALLOW_COPY_AND_ASSIGN(VirtualSceneRenderer);

public:
    ~VirtualSceneRenderer();

    // Create a VirtualSceneRenderer.
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    //
    // Returns a VirtualSceneRenderer instance if the renderer was successfully
    // created or null if there was an error.
    static std::unique_ptr<VirtualSceneRenderer> create(
            const GLESv2Dispatch* gles2);

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
    // Private constructor, use VirtualSceneRenderer::create to create an
    // instance.
    VirtualSceneRenderer(const GLESv2Dispatch* gles2);

    // Initial helper to initialize the VirtualSceneRenderer.
    bool initialize();

    const GLESv2Dispatch* const mGles2;

    VirtualSceneCamera mCamera;

    std::vector<std::unique_ptr<VirtualSceneTexture>> mCubeTextures;

    GLuint mProgram = 0;
    GLuint mVertexBuffer = 0;
    GLuint mIndexBuffer = 0;

    GLint mPositionLocation = -1;
    GLint mUvLocation = -1;
    GLint mTexSamplerLocation = -1;
    GLint mMvpLocation = -1;
};

}  // namespace virtualscene
}  // namespace android
