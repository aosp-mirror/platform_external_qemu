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
#include "android/virtualscene/VertexTypes.h"

#include <memory>
#include <vector>

namespace android {
namespace virtualscene {

struct Material {
    int id = -1;

    bool isValid() const { return id >= 0; }
};

struct Mesh {
    int id = -1;

    bool isValid() const { return id >= 0; }
};

struct Texture {
    int id = -1;

    bool isValid() const { return id >= 0; }
};

struct Renderable {
    Material material;
    Mesh mesh;
    Texture texture;
};

struct RenderableObject {
    glm::mat4 modelViewProj;
    Renderable renderable;
};

// Forward declarations.
class SceneObject;

class Renderer {
    DISALLOW_COPY_AND_ASSIGN(Renderer);

protected:
    Renderer();

public:
    virtual ~Renderer();

    // Create a virtual scene Renderer.
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    //
    // Returns a Renderer instance if the renderer was successfully created or
    // null if there was an error.
    static std::unique_ptr<Renderer> create(const GLESv2Dispatch* gles2,
                                            int width, int height);

    // Release rendering state associated with a SceneObject, should be called
    // before destroying a SceneObject.
    //
    // |parent| - SceneObject pointer, used as an opaque identifier.
    virtual void releaseObjectResources(const SceneObject* sceneObject) = 0;

    // Create a standard material for rendering with a texture. This is a
    // standard material and will be automatically cleaned up when the Renderer
    // is destroyed.
    //
    // Returns a Material instance or an invalid value if there was an error.
    virtual Material createMaterialTextured() = 0;

    // Create a material for rendering a screen-space effect with a custom
    // fragment shader, used by Effect.
    //
    // |parent| - Parent SceneObject, which controls the lifetime of the
    // material. |frag| - Frament shader to use for effect.
    //
    // Returns a Material instance or an invalid value if there was an error.
    virtual Material createMaterialScreenSpace(const SceneObject* parent,
                                               const char* frag) = 0;

    // Helper method to create a mesh given std::vectors containing the vertices
    // and indices.
    //
    // |parent| - Parent SceneObject, which controls the lifetime of the mesh.
    // |vertices| - Vertex array.
    // |indices| - Index array.
    //
    // Returns a Mesh instance or an invalid value if there was an error.
    Mesh createMesh(const SceneObject* parent,
                    const std::vector<VertexPositionUV>& vertices,
                    const std::vector<GLuint>& indices) {
        return createMesh(parent, vertices.data(), vertices.size(),
                          indices.data(), indices.size());
    }

    // Helper method to create a mesh given fixed-size arrays.
    //
    // |parent| - Parent SceneObject, which controls the lifetime of the mesh.
    // |vertices| - Vertex array.
    // |indices| - Index array.
    //
    // Returns a Mesh instance or an invalid value if there was an error.
    template <size_t verticesSize, size_t indicesSize>
    Mesh createMesh(const SceneObject* parent,
                    const VertexPositionUV (&vertices)[verticesSize],
                    const GLuint (&indices)[indicesSize]) {
        return createMesh(parent, vertices, verticesSize, indices, indicesSize);
    }

    // Create a mesh given a list of vertices and indices.
    //
    // |parent| - Parent SceneObject, which controls the lifetime of the mesh.
    // |vertices| - Pointer to an array of vertices.
    // |verticesSize| - Number of elements in the vertices array.
    // |indices| - Pointer to an array of indices.
    // |indicesSize| - Number of elements in the indices array.
    //
    // Returns a Mesh instance or an invalid value if there was an error.
    virtual Mesh createMesh(const SceneObject* parent,
                            const VertexPositionUV* vertices,
                            size_t verticesSize,
                            const GLuint* indices,
                            size_t indicesSize) = 0;

    // Load a PNG image from a file and create a Texture from it.
    //
    // |parent| - Parent SceneObject, which controls the lifetime of the
    // texture. |filename| - Filename to load.
    //
    // Returns a Texture instance or an invalid value if there was an error.
    virtual Texture loadTexture(const SceneObject* parent,
                                const char* filename) = 0;

    // Render a frame.
    virtual void render(const std::vector<RenderableObject>& renderables) = 0;
};

}  // namespace virtualscene
}  // namespace android
