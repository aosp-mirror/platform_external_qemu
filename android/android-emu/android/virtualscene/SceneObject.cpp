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

#include "android/virtualscene/SceneObject.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"
#include "android/virtualscene/Renderer.h"

#include <tiny_obj_loader.h>
#include <glm/gtx/hash.hpp>

#include <unordered_map>
#include <vector>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

using android::base::PathUtils;
using android::base::ScopedStdioFile;
using android::base::System;

namespace android {
namespace virtualscene {

static constexpr char kSimpleVertexShader[] = R"(
attribute vec3 in_position;
attribute vec2 in_uv;

uniform mat4 u_modelViewProj;

varying vec2 uv;

void main() {
    uv = in_uv;
    gl_Position = u_modelViewProj * vec4(in_position, 1.0);
}
)";

static constexpr char kSimpleFragmentShader[] = R"(
precision mediump float;
varying vec2 uv;

uniform sampler2D tex_sampler;

void main() {
  gl_FragColor = texture2D(tex_sampler, uv);
}
)";

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

bool operator==(const Vertex& lhs, const Vertex& rhs) {
    return lhs.pos == rhs.pos && lhs.uv == rhs.uv;
}

struct VertexHash {
    size_t operator()(const Vertex& v) const {
        return std::hash<glm::vec3>()(v.pos) ^ std::hash<glm::vec2>()(v.uv);
    }
};

SceneObject::SceneObject(Renderer& renderer,
                         GLuint vertexBuffer,
                         GLuint indexBuffer,
                         size_t indexCount)
    : mRenderer(renderer),
      mVertexBuffer(vertexBuffer),
      mIndexBuffer(indexBuffer),
      mIndexCount(indexCount) {}

SceneObject::~SceneObject() {
    const GLESv2Dispatch* gles2 = mRenderer.getGLESv2Dispatch();

    if (gles2) {
        // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
        gles2->glDeleteBuffers(1, &mVertexBuffer);
        gles2->glDeleteBuffers(1, &mIndexBuffer);
        gles2->glDeleteProgram(mProgram);
    }
}

bool SceneObject::initialize() {
    const GLESv2Dispatch* gles2 = mRenderer.getGLESv2Dispatch();

    // Compile and setup shaders.
    const GLuint vertexId =
            mRenderer.compileShader(GL_VERTEX_SHADER, kSimpleVertexShader);
    const GLuint fragmentId =
            mRenderer.compileShader(GL_FRAGMENT_SHADER, kSimpleFragmentShader);

    if (vertexId && fragmentId) {
        mProgram = mRenderer.linkShaders(vertexId, fragmentId);
    }

    // Release the shaders, the program will keep their reference alive.
    gles2->glDeleteShader(vertexId);
    gles2->glDeleteShader(fragmentId);

    if (!mProgram) {
        // Initialization failed, no program loaded.
        return false;
    }

    mPositionLocation = mRenderer.getAttribLocation(mProgram, "in_position");
    mUvLocation = mRenderer.getAttribLocation(mProgram, "in_uv");
    mTexSamplerLocation = mRenderer.getUniformLocation(mProgram, "tex_sampler");
    mMvpLocation = mRenderer.getUniformLocation(mProgram, "u_modelViewProj");

    return true;
}

std::unique_ptr<SceneObject> SceneObject::loadFromObj(Renderer& renderer,
                                                      const char* filename) {
    const std::string resourcesDir =
            PathUtils::join(System::get()->getLauncherDirectory(), "resources");
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

    const size_t vertexCount = attrib.vertices.size() / 3;
    const size_t texcoordCount = attrib.texcoords.size() / 2;

    std::vector<Vertex> vertices;
    std::unordered_map<Vertex, GLuint, VertexHash> existingVertexToIndex;
    std::vector<GLuint> indices;

    for (const tinyobj::shape_t& shape : shapes) {
        const tinyobj::mesh_t& mesh = shape.mesh;

        for (size_t i = 0; i < mesh.indices.size(); i++) {
            tinyobj::index_t index = mesh.indices[i];
            Vertex vertex;

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

                // Flip Y coord, OpenGL and .obj are reversed.
                vertex.uv = glm::vec2(
                        attrib.texcoords[2 * index.texcoord_index],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
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
    }

    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;
    const GLESv2Dispatch* gles2 = renderer.getGLESv2Dispatch();

    gles2->glGenBuffers(1, &vertexBuffer);
    gles2->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    gles2->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                        vertices.data(), GL_STATIC_DRAW);
    gles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    gles2->glGenBuffers(1, &indexBuffer);
    gles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    gles2->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                        indices.size() * sizeof(GLuint), indices.data(),
                        GL_STATIC_DRAW);
    gles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    D("%s: Creating scene object with %d vertices, %d indices", __FUNCTION__,
      vertices.size(), indices.size());

    std::unique_ptr<SceneObject> result(new SceneObject(
            renderer, vertexBuffer, indexBuffer, indices.size()));
    if (!result->initialize()) {
        return nullptr;
    }

    return result;
}

void SceneObject::setTexture(std::unique_ptr<Texture>&& texture) {
    mTexture = std::move(texture);
}

void SceneObject::setTransform(const glm::mat4& transform) {
    mTransform = transform;
}

glm::mat4 SceneObject::getTransform() const {
    return mTransform;
}

void SceneObject::render() {
    const GLESv2Dispatch* gles2 = mRenderer.getGLESv2Dispatch();

    gles2->glUseProgram(mProgram);

    // Bind the texture if it exists.
    if (mTexture) {
        gles2->glActiveTexture(GL_TEXTURE0);
        gles2->glUniform1i(mTexSamplerLocation, 0);

        gles2->glBindTexture(GL_TEXTURE_2D, mTexture->getTextureId());
    } else {
        gles2->glBindTexture(GL_TEXTURE_2D, 0);
    }

    const glm::mat4 mvp =
            mRenderer.getCamera().getViewProjection() * mTransform;
    gles2->glUniformMatrix4fv(mMvpLocation, 1, GL_FALSE, &mvp[0][0]);

    gles2->glEnableVertexAttribArray(mPositionLocation);
    gles2->glEnableVertexAttribArray(mUvLocation);

    gles2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    gles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

    // glVertexAttribPointer requires the GL_ARRAY_BUFFER to be bound, keep
    // it bound until all of the vertex attribs are initialized.
    if (mPositionLocation >= 0) {
        gles2->glVertexAttribPointer(
                mPositionLocation,
                3,                                                // size
                GL_FLOAT,                                         // type
                GL_FALSE,                                         // normalized?
                sizeof(Vertex),                                   // stride
                reinterpret_cast<void*>(offsetof(Vertex, pos)));  // offset
    }

    if (mUvLocation >= 0) {
        gles2->glVertexAttribPointer(
                mUvLocation,
                2,                                               // size
                GL_FLOAT,                                        // type
                GL_FALSE,                                        // normalized?
                sizeof(Vertex),                                  // stride
                reinterpret_cast<void*>(offsetof(Vertex, uv)));  // offset
    }

    gles2->glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, nullptr);
    gles2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    gles2->glDisableVertexAttribArray(mPositionLocation);
    gles2->glDisableVertexAttribArray(mUvLocation);
    gles2->glBindTexture(GL_TEXTURE_2D, 0);
    gles2->glUseProgram(0);
}

}  // namespace virtualscene
}  // namespace android
