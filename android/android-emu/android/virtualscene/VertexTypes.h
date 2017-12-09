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
 * Defines Vertex types used by the Virtual Scene Renderer.
 */

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace android {
namespace virtualscene {

struct VertexPositionUV {
    glm::vec3 pos;
    glm::vec2 uv;
};

inline bool operator==(const VertexPositionUV& lhs,
                       const VertexPositionUV& rhs) {
    return lhs.pos == rhs.pos && lhs.uv == rhs.uv;
}

struct VertexPositionUVHash {
    size_t operator()(const VertexPositionUV& v) const {
        return std::hash<glm::vec3>()(v.pos) ^ std::hash<glm::vec2>()(v.uv);
    }
};

struct VertexInfo {
    bool hasUV = false;
    size_t vertexSize = 0;
    size_t posOffset = 0;
    size_t uvOffset = 0;
};

}  // namespace virtualscene
}  // namespace android
