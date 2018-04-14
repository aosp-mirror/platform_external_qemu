/*
 * Copyright (C) 2018 The Android Open Source Project
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
 * Defines PosterSceneObject, which represents a quad in the 3D scene.
 */

#include "android/virtualscene/SceneObject.h"

namespace android {
namespace virtualscene {

class PosterSceneObject : public SceneObject {
    DISALLOW_COPY_AND_ASSIGN(PosterSceneObject);

protected:
    PosterSceneObject(Renderer& renderer);

public:
    // Creates a PosterSceneObject from a unit quad.
    //
    // |renderer| - Renderer context, VirtualPosterSceneObject will be bound to
    //              this context.
    // |position| - Position of the poster, in world space.
    // |rotation| - Rotation quaternion.
    // |minSize| - Minimum size of any dimension of the poster, in meters.
    // |maxSize| - Maximum size of the poster, in meters.  This will be scaled
    //             down along the smaller axis of the texture, if it is not
    //             square.
    //
    // Returns a PosterSceneObject instance if the object could be loaded or
    // null if there was an error.
    static std::unique_ptr<PosterSceneObject> create(Renderer& renderer,
                                                     const glm::vec3& position,
                                                     const glm::quat& rotation,
                                                     float minSize,
                                                     const glm::vec2& maxSize);

    // Update the texture in the PosterSceneObject.
    //
    // |texture| - Texture object.
    void setTexture(Texture texture);

    // Set the scale of the poster, with a value between 0 and 1.  The value
    // will be clamped between the minimum poster size, 20cm and the maximum
    // poster size as defined in the scene (typically 1-2m).
    //
    // |value| - Scale of the poster.
    void setScale(float value);

    // Update the PosterSceneObject for the current frame.
    void update();

private:
    // Make private, setScale should be used instead.
    using SceneObject::setTransform;

    void updateTransform();

    // Set in create().
    glm::mat4 mPositionRotation = glm::mat4();
    float mMinSize = 0.0f;
    glm::vec2 mMaxSize;

    // Updated each frame if the texture has changed.
    float mMinScale = 0.0f;
    glm::mat4 mPosterSizeTransform = glm::mat4();

    // Dynamically adjustable with setScale().
    float mScale = 1.0f;
};

}  // namespace virtualscene
}  // namespace android
