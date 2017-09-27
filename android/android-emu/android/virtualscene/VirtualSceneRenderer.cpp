/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "android/virtualscene/VirtualSceneRenderer.h"

#include "android/utils/debug.h"
#include <vector>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(camera)

/*******************************************************************************
 *                     Renderer routines
 ******************************************************************************/

static const char g_shader_vertex[] = R"(
attribute vec3 in_position;

void main() {
    gl_Position = vec4(in_position, 1.0);
}
)";

static const char g_shader_fragment[] = R"(
precision mediump float;

void main() {
  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

// clang-format off
// Vertices for a simple screen-space triangle.
static const GLfloat g_vertex_buffer_data[] = {
     1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
     0.0f, -1.0f, 0.0f,
};
// clang-format on

class VirtualSceneRendererImpl {
public:
    VirtualSceneRendererImpl(const GLESv2Dispatch* gles2) : gles2_(gles2) {
        program_ = loadShaders(g_shader_vertex, g_shader_fragment);

        gles2_->glGenBuffers(1, &vertex_buffer_);
        gles2_->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        gles2_->glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data),
                             g_vertex_buffer_data, GL_STATIC_DRAW);

        const GLint position_handle =
                gles2->glGetAttribLocation(program_, "in_position");

        if (position_handle >= 0) {
            gles2->glEnableVertexAttribArray(position_handle);
            gles2_->glVertexAttribPointer(position_handle,
                                          3,                     // size
                                          GL_FLOAT,              // type
                                          GL_FALSE,              // normalized?
                                          0,                     // stride
                                          static_cast<void*>(0)  // offset
                                          );
        }
    }

    ~VirtualSceneRendererImpl() {
        // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
        gles2_->glDeleteBuffers(1, &vertex_buffer_);
        vertex_buffer_ = 0;

        gles2_->glDeleteProgram(program_);
        program_ = 0;
    }

    GLuint loadShaders(const char* vertex_shader, const char* fragment_shader) {
        const GLuint vertex_id = gles2_->glCreateShader(GL_VERTEX_SHADER);
        const GLuint fragment_id = gles2_->glCreateShader(GL_FRAGMENT_SHADER);

        GLint result = GL_FALSE;

        // Vertex shader.
        gles2_->glShaderSource(vertex_id, 1, &vertex_shader, nullptr);
        gles2_->glCompileShader(vertex_id);

        // Output the error message if the compile failed.
        gles2_->glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE) {
            int log_length = 0;
            gles2_->glGetShaderiv(vertex_id, GL_INFO_LOG_LENGTH, &log_length);

            if (log_length) {
                std::vector<char> message(log_length + 1);
                gles2_->glGetShaderInfoLog(vertex_id, log_length, nullptr,
                                           message.data());
                E("%s: Failed to compile vertex shader: %s", __FUNCTION__,
                  message.data());
            } else {
                E("%s: Failed to compile vertex shader", __FUNCTION__);
            }
        }

        // Fragment shader.
        if (result == GL_TRUE) {
            gles2_->glShaderSource(fragment_id, 1, &fragment_shader, nullptr);
            gles2_->glCompileShader(fragment_id);

            // Output the error message if the compile failed.
            gles2_->glGetShaderiv(fragment_id, GL_COMPILE_STATUS, &result);
            if (result != GL_TRUE) {
                int log_length = 0;
                gles2_->glGetShaderiv(fragment_id, GL_INFO_LOG_LENGTH,
                                      &log_length);

                if (log_length) {
                    std::vector<char> message(log_length + 1);
                    gles2_->glGetShaderInfoLog(fragment_id, log_length, nullptr,
                                               message.data());
                    E("%s: Failed to compile fragment shader: %s", __FUNCTION__,
                      message.data());
                } else {
                    E("%s: Failed to compile fragment shader", __FUNCTION__);
                }
            }
        }

        GLuint program_id = 0;

        if (result == GL_TRUE) {
            program_id = gles2_->glCreateProgram();
            gles2_->glAttachShader(program_id, vertex_id);
            gles2_->glAttachShader(program_id, fragment_id);
            gles2_->glLinkProgram(program_id);

            // Output the error message if the program failed to link.
            gles2_->glGetProgramiv(program_id, GL_LINK_STATUS, &result);
            if (result != GL_TRUE) {
                int log_length = 0;
                gles2_->glGetProgramiv(program_id, GL_INFO_LOG_LENGTH,
                                       &log_length);

                if (log_length) {
                    std::vector<char> message(log_length + 1);
                    gles2_->glGetProgramInfoLog(fragment_id, log_length,
                                                nullptr, message.data());
                    E("%s: Failed to link shader: %s", __FUNCTION__,
                      message.data());
                } else {
                    E("%s: Failed to link shader", __FUNCTION__);
                }

                gles2_->glDeleteProgram(program_id);
                program_id = 0;
            } else {
                gles2_->glDetachShader(program_id, vertex_id);
                gles2_->glDetachShader(program_id, fragment_id);
            }
        }

        gles2_->glDeleteShader(vertex_id);
        gles2_->glDeleteShader(fragment_id);

        return program_id;
    }

    void render() {
        color_r_ += 0.02f;
        if (color_r_ > 1.0f) {
            color_r_ = 0.2f;
        }

        gles2_->glClearColor(color_r_, 0.0f, 0.0f, 1.0f);
        gles2_->glClear(GL_COLOR_BUFFER_BIT);

        gles2_->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

        // Draw the triangle!
        gles2_->glUseProgram(program_);
        gles2_->glDrawArrays(
                GL_TRIANGLES, 0,
                3);  // Starting from vertex 0; 3 vertices total -> 1 triangle

        gles2_->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

private:
    const GLESv2Dispatch* const gles2_;

    GLuint program_ = 0;
    GLuint vertex_buffer_ = 0;

    float color_r_ = 0.2f;
};

/*******************************************************************************
 *                     Renderer wrapper
 ******************************************************************************/

std::mutex VirtualSceneRenderer::lock_;
VirtualSceneRendererImpl* VirtualSceneRenderer::impl_ = nullptr;

void VirtualSceneRenderer::initialize(const GLESv2Dispatch* gles2) {
    std::lock_guard<std::mutex> lock(lock_);
    if (impl_) {
        E("VirtualSceneRenderer already initialized");
        return;
    }

    impl_ = new VirtualSceneRendererImpl(gles2);
}

void VirtualSceneRenderer::uninitialize() {
    std::lock_guard<std::mutex> lock(lock_);
    if (!impl_) {
        E("VirtualSceneRenderer not initialized");
        return;
    }

    delete impl_;
    impl_ = nullptr;
}

void VirtualSceneRenderer::render() {
    std::lock_guard<std::mutex> lock(lock_);
    if (!impl_) {
        E("VirtualSceneRenderer not initialized");
        return;
    }

    impl_->render();
}
