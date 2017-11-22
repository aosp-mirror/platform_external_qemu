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
 * Defines SceneObject, which represents an object in the 3D scene.
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Compiler.h"
#include "android/utils/compiler.h"
#include "android/virtualscene/Texture.h"

#include <glm/glm.hpp>

namespace android {
namespace virtualscene {

// Forward declarations.
class Renderer;

class SceneObject {
    DISALLOW_COPY_AND_ASSIGN(SceneObject);

public:
    ~SceneObject();

    // Loads an object mesh from an .obj file.
    //
    // |renderer| - Renderer context, VirtualSceneObject will be bound to this
    //              context.
    // |filename| - Filename to load.
    //
    // Returns a SceneObject instance if the object could be loaded or null if
    // there was an error.
    static std::unique_ptr<SceneObject> loadFromObj(Renderer& renderer,
                                                    const char* filename);

    // Sets the model transform.
    //
    // |transform| - Model transform for this object.
    void setTransform(const glm::mat4& transform);

    // Gets the model transform for this object.
    glm::mat4 getTransform() const;

    // Sets the default texture for the object.
    //
    // |texture| - Texture object, SceneObject will take ownership and the
    //             caller's version will be empty after this call.
    void setTexture(std::unique_ptr<Texture>&& texture);

    // Render the object.
    void render();

private:
    // Private constructor, use SceneObject::loadFromObj to create an
    // instance.
    SceneObject(Renderer& renderer,
                GLuint vertexBuffer,
                GLuint indexBuffer,
                size_t indexCount);

    bool initialize();

    Renderer& mRenderer;

    glm::mat4 mTransform = glm::mat4();

    GLuint mVertexBuffer = 0;
    GLuint mIndexBuffer = 0;
    size_t mIndexCount = 0;
    std::unique_ptr<Texture> mTexture;

    GLuint mProgram = 0;

    GLint mPositionLocation = -1;
    GLint mUvLocation = -1;
    GLint mTexSamplerLocation = -1;
    GLint mMvpLocation = -1;
};

}  // namespace virtualscene
}  // namespace android
