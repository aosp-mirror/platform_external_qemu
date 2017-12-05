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
#include "android/virtualscene/Material.h"
#include "android/virtualscene/Mesh.h"
#include "android/virtualscene/RenderTarget.h"
#include "android/virtualscene/SceneCamera.h"
#include "android/virtualscene/Texture.h"
#include "android/virtualscene/VertexTypes.h"

#include <functional>
#include <vector>

namespace android {
namespace virtualscene {

struct RenderCommand {
    std::shared_ptr<Material> material;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Mesh> mesh;
};

// Forward declarations.
class SceneObject;
class Effect;

using RenderCommandParameterCallback =
        std::function<void(const std::shared_ptr<Material>& material)>;

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
    static std::unique_ptr<Renderer> create(const GLESv2Dispatch* gles2,
                                            int width, int height);

    const SceneCamera& getCamera() const;
    const GLESv2Dispatch* getGLESv2Dispatch();

    // Create a standard material for rendering with a texture
    std::shared_ptr<Material> createMaterialTextured();

    // Create a material for rendering a screen-space effect with a custom
    // fragment shader, used by Effect.
    //
    // |frag| - Frament shader to use for effect.
    //
    // Returns a Material instance if successful or null if there was an error.
    std::shared_ptr<Material> createMaterialScreenSpace(const char* frag);

    // Helper method to create a mesh given std::vectors containing the vertices
    // and indices.
    //
    // |vertices| - Vertex array.
    // |indices| - Index array.
    //
    // Return a Mesh instance if successful or null if there was an error.
    std::shared_ptr<Mesh> createMesh(
            const std::vector<VertexPositionUV>& vertices,
            const std::vector<GLuint>& indices) {
        return createMesh(vertices.data(), vertices.size(), indices.data(),
                          indices.size());
    }

    // Helper method to create a mesh given fixed-size arrays.
    //
    // |vertices| - Vertex array.
    // |indices| - Index array.
    //
    // Return a Mesh instance if successful or null if there was an error.
    template <size_t verticesSize, size_t indicesSize>
    std::shared_ptr<Mesh> createMesh(
            const VertexPositionUV (&vertices)[verticesSize],
            const GLuint (&indices)[indicesSize]) {
        return createMesh(vertices, verticesSize, indices, indicesSize);
    }

    // Create a mesh given a list of vertices and indices.
    //
    // |vertices| - Pointer to an array of vertices.
    // |verticesSize| - Number of elements in the vertices array.
    // |indices| - Pointer to an array of indices.
    // |indicesSize| - Number of elements in the indices array.
    //
    // Return a Mesh instance if successful or null if there was an error.
    std::shared_ptr<Mesh> createMesh(const VertexPositionUV* vertices,
                                     size_t verticesSize,
                                     const GLuint* indices,
                                     size_t indicesSize);

    // Render a frame.
    //
    // Returns the timestamp at which the frame was rendered.
    int64_t render();

private:
    // Private constructor, use Renderer::create to create an
    // instance.
    Renderer(const GLESv2Dispatch* gles2);

    // Initial helper to initialize the Renderer.
    bool initialize(int width, int height);

    // Helper to create a SceneObject from an obj file.
    // |objFile| - Filename of the obj file to load.
    //
    // Returns the SceneObject is loading was successful, or null otherwise.
    std::unique_ptr<SceneObject> loadSceneObject(const char* objFile);

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

    // Executes the given render command, used internally by Renderer::render.
    void renderCommandExecute(const RenderCommand& renderCommand,
                              RenderCommandParameterCallback parameterCallback);

    const GLESv2Dispatch* const mGles2;

    SceneCamera mCamera;

    std::vector<std::unique_ptr<SceneObject>> mSceneObjects;

    int mRenderWidth;
    int mRenderHeight;

    std::unique_ptr<RenderTarget> mRenderTargets[2];
    std::unique_ptr<RenderTarget> mScreenRenderTarget;

    std::vector<std::unique_ptr<Effect>> mEffectsChain;

    std::shared_ptr<Material> mMaterialTextured;
};

}  // namespace virtualscene
}  // namespace android
