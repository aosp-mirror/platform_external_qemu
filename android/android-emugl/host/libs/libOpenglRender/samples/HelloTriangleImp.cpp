// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file provides implementation for the sample app HelloTriangle.
// The main executable source is HelloTriangle.cpp.
#include "HelloTriangle.h"

#include "android/base/system/System.h"

#include "Standalone.h"

#include <functional>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using android::base::System;

namespace emugl {

void HelloTriangle::initialize() {
    constexpr char vshaderSrc[] = R"(#version 300 es
    precision highp float;

    layout (location = 0) in vec2 pos;
    layout (location = 1) in vec3 color;

    uniform mat4 transform;

    out vec3 color_varying;

    void main() {
        gl_Position = transform * vec4(pos, 0.0, 1.0);
        color_varying = (transform * vec4(color, 1.0)).xyz;
    }
    )";
    constexpr char fshaderSrc[] = R"(#version 300 es
    precision highp float;

    in vec3 color_varying;

    out vec4 fragColor;

    void main() {
        fragColor = vec4(color_varying, 1.0);
    }
    )";

    GLint program = emugl::compileAndLinkShaderProgram(vshaderSrc, fshaderSrc);

    auto gl = getGlDispatch();

    mTransformLoc = gl->glGetUniformLocation(program, "transform");

    gl->glEnableVertexAttribArray(0);
    gl->glEnableVertexAttribArray(1);

    const VertexAttributes vertexAttrs[] = {
        { { -0.5f, -0.5f,}, { 0.2, 0.1, 0.9, }, },
        { { 0.5f, -0.5f,}, { 0.8, 0.3, 0.1,}, },
        { { 0.0f, 0.5f,}, { 0.1, 0.9, 0.6,}, },
    };

    gl->glGenBuffers(1, &mBuffer);
    gl->glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrs), vertexAttrs,
                     GL_STATIC_DRAW);

    gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              sizeof(VertexAttributes), 0);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              sizeof(VertexAttributes),
                              (GLvoid*)offsetof(VertexAttributes, color));

    gl->glUseProgram(program);

    gl->glClearColor(0.2f, 0.2f, 0.3f, 0.0f);
}

void HelloTriangle::draw() {
    glm::mat4 rot =
            glm::rotate(glm::mat4(), mTime, glm::vec3(0.0f, 0.0f, 1.0f));

    auto gl = getGlDispatch();

    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl->glUniformMatrix4fv(mTransformLoc, 1, GL_FALSE, glm::value_ptr(rot));
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);

    mTime += 0.05f;
}

}  // namespace emugl
