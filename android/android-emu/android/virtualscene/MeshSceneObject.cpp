// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/virtualscene/MeshSceneObject.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"

#include <tiny_obj_loader.h>

#include <unordered_map>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

using android::base::PathUtils;
using android::base::System;

namespace android {
namespace virtualscene {

MeshSceneObject::MeshSceneObject(Renderer& renderer) : SceneObject(renderer) {}

std::unique_ptr<MeshSceneObject> MeshSceneObject::load(Renderer& renderer,
                                                       const char* filename) {
    const std::string resourcesDir =
            PathUtils::addTrailingDirSeparator(PathUtils::join(
                    System::get()->getLauncherDirectory(), "resources"));
    const std::string filePath = PathUtils::join(
            System::get()->getLauncherDirectory(), "resources", filename);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;
    const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
                                      filePath.c_str(), resourcesDir.c_str());
    if (!ret) {
        E("%s: Error loading obj %s: %s", __FUNCTION__, filename,
          err.empty() ? "<no message>" : err.c_str());
        return nullptr;
    } else if (!err.empty()) {
        W("%s: Warnings loading obj %s: %s", __FUNCTION__, filename,
          err.c_str());
    }

    std::unique_ptr<MeshSceneObject> result(new MeshSceneObject(renderer));

    const size_t vertexCount = attrib.vertices.size() / 3;
    const size_t texcoordCount = attrib.texcoords.size() / 2;

    for (const tinyobj::shape_t& shape : shapes) {
        const tinyobj::mesh_t& mesh = shape.mesh;

        std::vector<VertexPositionUV> vertices;
        std::unordered_map<VertexPositionUV, GLuint, VertexPositionUVHash>
                existingVertexToIndex;
        std::vector<GLuint> indices;
        Texture texture;

        bool useCheckerboardMaterial = false;

        if (!mesh.material_ids.empty()) {
            const int material_id = mesh.material_ids[0];
            if (material_id >= 0 && material_id < materials.size()) {
                if (strstr(materials[material_id].diffuse_texname.c_str(),
                           "TV")) {
                    useCheckerboardMaterial = true;
                } else {
                    texture = renderer.loadTexture(
                            materials[material_id].diffuse_texname.c_str());
                }
            }
        }

        for (size_t i = 0; i < mesh.indices.size(); i++) {
            tinyobj::index_t index = mesh.indices[i];
            VertexPositionUV vertex;

            if (index.vertex_index < 0 || index.vertex_index >= vertexCount) {
                E("%s: Error parsing %s, invalid vertex index %d, expected "
                  "less than %d",
                  __FUNCTION__, filename, index.vertex_index, vertexCount);
                return nullptr;
            }

            vertex.pos = glm::vec3(attrib.vertices[3 * index.vertex_index],
                                   attrib.vertices[3 * index.vertex_index + 1],
                                   attrib.vertices[3 * index.vertex_index + 2]);

            if (index.texcoord_index >= 0) {
                if (index.texcoord_index >= texcoordCount) {
                    E("%s: Error parsing %s, invalid vertex index %d, expected "
                      "less than %d",
                      __FUNCTION__, filename, index.texcoord_index,
                      texcoordCount);
                    return nullptr;
                }

                vertex.uv = glm::vec2(
                        attrib.texcoords[2 * index.texcoord_index],
                        attrib.texcoords[2 * index.texcoord_index + 1]);
            }

            auto existingEntry = existingVertexToIndex.find(vertex);
            if (existingEntry != existingVertexToIndex.end()) {
                indices.push_back(existingEntry->second);
            } else {
                vertices.push_back(vertex);

                const GLuint index = vertices.size() - 1;
                indices.push_back(index);

                existingVertexToIndex[vertex] = index;
            }
        }

        D("%s: Creating mesh with %d vertices, %d indices", __FUNCTION__,
          vertices.size(), indices.size());

        Renderable renderable;
        renderable.material = useCheckerboardMaterial
                                      ? renderer.createMaterialCheckerboard()
                                      : renderer.createMaterialTextured();
        renderable.mesh = renderer.createMesh(vertices, indices);
        renderable.texture = texture;

        result.get()->mRenderables.emplace_back(std::move(renderable));
    }

    return result;
}

}  // namespace virtualscene
}  // namespace android
