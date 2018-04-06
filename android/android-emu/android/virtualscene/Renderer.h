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
    bool operator==(const Texture& rhs) const { return id == rhs.id; }
    bool operator!=(const Texture& rhs) const { return id != rhs.id; }
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

    // Get the aspect ratio of the frame, w/h.
    virtual float getAspectRatio() = 0;

    // Determine if a texture has finished loading, for use with async texture
    // loads.
    //
    // |texture| - Texture object.
    //
    // Returns true if the texture has finished loading.  If the texture was not
    // created with loadTextureAsync, always returns true.
    virtual bool isTextureLoaded(Texture texture) = 0;

    // Get the dimensions of a texture.
    //
    // |texture| - Texture object.
    // |outWidth| - Out parameter that returns texture width, in pixels.
    // |outHeight| - Out parameter that returns texture height, in pixels.
    //
    // Returns a width and height for a texture.
    virtual void getTextureInfo(Texture texture,
                                uint32_t* outWidth,
                                uint32_t* outHeight) = 0;

    // Release a texture. If this is the last reference, the texture resources
    // will be released.
    //
    // |texture| - Texture object.
    virtual void releaseTexture(Texture texture) = 0;

    // Release a material.
    //
    // |material| - Material object.
    virtual void releaseMaterial(Material material) = 0;

    // Release a mesh.
    //
    // |Mesh| - Mesh object.
    virtual void releaseMesh(Mesh mesh) = 0;

    // Create a standard material for rendering a test checkerboard texture.
    // This is a standard material and will be automatically cleaned up when the
    // Renderer is destroyed.
    //
    // Returns a Material instance or an invalid value if there was an error.
    virtual Material createMaterialCheckerboard() = 0;

    // Create a standard material for rendering with a texture. This is a
    // standard material and will be automatically cleaned up when the Renderer
    // is destroyed.
    //
    // Returns a Material instance or an invalid value if there was an error.
    virtual Material createMaterialTextured() = 0;

    // Create a material for rendering a screen-space effect with a custom
    // fragment shader, used by Effect.
    //
    // |frag| - Frament shader to use for effect.
    //
    // Returns a Material instance or an invalid value if there was an error.
    virtual Material createMaterialScreenSpace(const char* frag) = 0;

    // Helper method to create a mesh given std::vectors containing the vertices
    // and indices.
    //
    // |vertices| - Vertex array.
    // |indices| - Index array.
    //
    // Returns a Mesh instance or an invalid value if there was an error.
    Mesh createMesh(const std::vector<VertexPositionUV>& vertices,
                    const std::vector<GLuint>& indices) {
        return createMesh(vertices.data(), vertices.size(), indices.data(),
                          indices.size());
    }

    // Helper method to create a mesh given fixed-size arrays.
    //
    // |vertices| - Vertex array.
    // |indices| - Index array.
    //
    // Returns a Mesh instance or an invalid value if there was an error.
    template <size_t verticesSize, size_t indicesSize>
    Mesh createMesh(const VertexPositionUV (&vertices)[verticesSize],
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
    // Returns a Mesh instance or an invalid value if there was an error.
    virtual Mesh createMesh(const VertexPositionUV* vertices,
                            size_t verticesSize,
                            const GLuint* indices,
                            size_t indicesSize) = 0;

    // Load an image from a file and create a Texture from it.
    //
    // |filename| - Filename to load.
    //
    // Returns a Texture. If there was an error, the Texture will be invalid,
    // which can be queried with Texture::isValid().
    //
    // NOTE: If the same texture is loaded with loadTextureAsync and has not
    // finished loading, the returned texture may contain a placeholder texture
    // until loading has completed.  To avoid this, do not mix-and-match
    // loadTexture and loadTextureAsync.
    virtual Texture loadTexture(const char* filename) = 0;

    // Load an image from a file and create a Texture from it.
    //
    // The Texture will be immediately created, but will initially contain a
    // transparent placeholder texture.  The actual texture will automatically
    // load in the background.
    //
    // To check if a texture has finished loading, use isTextureLoaded().
    //
    // |filename| - Filename to load.
    //
    // Returns a Texture. If there was an error, the Texture will be invalid,
    // which can be queried with Texture::isValid().
    virtual Texture loadTextureAsync(const char* filename) = 0;

    // Duplicate a Texture instance.
    //
    // |texture| - Texture object.
    //
    // Returns a texture object with one additional reference.
    virtual Texture duplicateTexture(Texture texture) = 0;

    // Render a frame.
    virtual void render(const std::vector<RenderableObject>& renderables,
                        float time) = 0;
};

}  // namespace virtualscene
}  // namespace android
