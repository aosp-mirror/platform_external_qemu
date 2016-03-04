// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/gl-anti-aliasing.h"

#include "android/skin/qt/gl-common.h"
#include "GLES2/gl2.h"

#include <cstring>

// Vertex shader for anti-aliasing - doesn't do anything special.
static const char AAVertexShaderSource[] = R"(
    attribute vec3 position;
    void main(void) {
        gl_Position = vec4(position, 1.0);
    })";

// Fragment shader - does fast approximate anti-aliasing (FXAA)
// Based on:
// http://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/
static const char AAFragmentShaderSource[] = R"(
    uniform sampler2D texture;
    uniform vec2 resolution;
    float luma(vec3 color) {
        return dot(color, vec3(0.299, 0.587, 0.114));
    }
    vec4 antialias(sampler2D tex,
                   vec2 pixel_coord,
                   vec2 inverse_res,
                   vec2 nw, vec2 ne,
                   vec2 sw, vec2 se,
                   vec2 m) {
        vec4 m_rgba = texture2D(tex, m);
        vec3 nw_color = texture2D(tex, nw).xyz;
        vec3 ne_color = texture2D(tex, ne).xyz;
        vec3 sw_color = texture2D(tex, sw).xyz;
        vec3 se_color = texture2D(tex, se).xyz;
        vec3 m_color  = m_rgba.xyz;
        float nw_luma = luma(nw_color);
        float ne_luma = luma(ne_color);
        float sw_luma = luma(sw_color);
        float se_luma = luma(se_color);
        float m_luma  = luma(m_color);
        float min_luma = min(m_luma, min(min(nw_luma, ne_luma), min(sw_luma, se_luma)));
        float max_luma = max(m_luma, max(max(nw_luma, ne_luma), max(sw_luma, se_luma)));

        mediump vec2 dir;
        dir.x = -((nw_luma + ne_luma) - (sw_luma + se_luma));
        dir.y =  ((nw_luma + sw_luma) - (ne_luma + se_luma));

        float dir_reduce = max((nw_luma + ne_luma + sw_luma + se_luma) *
                              (0.25 * (1.0 / 8.0)), (1.0/256.0));

        float rcp_dir_min = 1.0 / (min(abs(dir.x), abs(dir.y)) + dir_reduce);
        dir = min(vec2(16.0, 16.0),
                  max(vec2(-16.0, -16.0),
                  dir * rcp_dir_min)) * inverse_res;

        vec2 tex_coord = pixel_coord * inverse_res;
        vec3 rgb_a = 0.5 * (
            texture2D(tex, tex_coord + dir * (1.0 / 3.0 - 0.5)).xyz +
            texture2D(tex, tex_coord + dir * (2.0 / 3.0 - 0.5)).xyz);
        vec3 rgb_b = rgb_a * 0.5 + 0.25 * (
            texture2D(tex, tex_coord + dir * -0.5).xyz +
            texture2D(tex, tex_coord+ dir * 0.5).xyz);

        float luma_b = luma(rgb_b);
        return vec4((luma_b < min_luma) || (luma_b > max_luma) ? rgb_a  : rgb_a, m_rgba.a);
    }
    void main(void) {
        vec2 pixel_coord = gl_FragCoord.xy;
        vec2 inverse_res = 1.0 / resolution;
        vec2 nw = (pixel_coord + vec2(-1.0, -1.0)) * inverse_res;
        vec2 ne = (pixel_coord + vec2( 1.0, -1.0)) * inverse_res;
        vec2 sw = (pixel_coord + vec2(-1.0,  1.0)) * inverse_res;
        vec2 se = (pixel_coord + vec2( 1.0,  1.0)) * inverse_res;
        vec2 m = pixel_coord * inverse_res;
        gl_FragColor = antialias(texture, pixel_coord, inverse_res, nw, ne, sw, se, m);
    }
)";

// Helper for GLAntiAliasing ctor.
static GLuint createShader(const GLESv2Dispatch* gles2,
                           GLint shaderType,
                           const char* shaderText) {
    GLuint shader = gles2->glCreateShader(shaderType);
    if (!shader) {
        return 0;
    }
    const GLchar* text = static_cast<const GLchar*>(shaderText);
    const GLint text_len = strlen(shaderText);
    gles2->glShaderSource(shader, 1, &text, &text_len);
    gles2->glCompileShader(shader);
    GLint success;
    gles2->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char msgs[256];
        gles2->glGetShaderInfoLog(shader, sizeof(msgs), nullptr, msgs);
        fprintf(stderr, "Error compiling shader %s", msgs);
    }
    return success == GL_TRUE ? shader : 0;
}

GLAntiAliasing::GLAntiAliasing(const GLESv2Dispatch* gl_dispatch) :
        mGLES2(gl_dispatch),
        mAAProgram(0),
        mAAVertexBuffer(0) {
    // Build the anti-aliasing shader.
    GLuint vertex_shader =
            createShader(mGLES2, GL_VERTEX_SHADER, AAVertexShaderSource);
    CHECK_GL_ERROR("Failed to create vertex shader for anti-aliasing");
    GLuint fragment_shader =
            createShader(mGLES2, GL_FRAGMENT_SHADER, AAFragmentShaderSource);
    CHECK_GL_ERROR("Failed to create fragment shaderText for anti-aliasing");

    mAAProgram = mGLES2->glCreateProgram();
    CHECK_GL_ERROR("Failed to create program object");
    mGLES2->glAttachShader(mAAProgram, vertex_shader);
    mGLES2->glAttachShader(mAAProgram, fragment_shader);
    mGLES2->glLinkProgram(mAAProgram);

    // Shader objects no longer needed.
    mGLES2->glDeleteShader(vertex_shader);
    mGLES2->glDeleteShader(fragment_shader);

    // Check for errors.
    GLint success;
    mGLES2->glGetProgramiv(mAAProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infolog[256];
        mGLES2->glGetProgramInfoLog(mAAProgram, sizeof(infolog), 0, infolog);
        fprintf(stderr, "Could not create/link program: %s\n", infolog);
        return;
    }

    // Get all the attributes and uniforms.
    mAAPositionAttribLocation =
            mGLES2->glGetAttribLocation(mAAProgram, "position");
    mAAInputUniformLocation =
            mGLES2->glGetUniformLocation(mAAProgram, "texture");
    mAAResolutionUniformLocation =
            mGLES2->glGetUniformLocation(mAAProgram, "resolution");
    CHECK_GL_ERROR("Failed to get attributes & uniforms for shader");

    // Create vertex and index buffers.
    mGLES2->glGenBuffers(1, &mAAVertexBuffer);
    CHECK_GL_ERROR("Failed to create vertex buffer for anti-aliasing shader");

    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mAAVertexBuffer);
    static const float vertex_data[] = {
            -1.0, -1.0, 0.0, 3.0, -1.0, 0.0, -1.0, 3.0, 0.0,
    };
    mGLES2->glBufferData(GL_ARRAY_BUFFER,
                         sizeof(vertex_data),
                         vertex_data,
                         GL_STATIC_DRAW);
    CHECK_GL_ERROR("Failed to populate vertex buffer");
}

GLAntiAliasing::~GLAntiAliasing() {
    if (mAAProgram) {
        mGLES2->glUseProgram(0);
        mGLES2->glDeleteProgram(mAAProgram);
        mAAProgram = 0;
    }

    if (mAAVertexBuffer) {
        mGLES2->glBindBuffer(GL_ARRAY_BUFFER, 0);
        mGLES2->glDeleteBuffers(1, &mAAVertexBuffer);
        mAAVertexBuffer = 0;
    }
}

void GLAntiAliasing::apply(GLuint input_texture, int width, int height) {
    // Draw texture on-screen, applying the anti-aliasing shader.
    mGLES2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mGLES2->glUseProgram(mAAProgram);
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mAAVertexBuffer);
    mGLES2->glEnableVertexAttribArray(mAAPositionAttribLocation);
    mGLES2->glVertexAttribPointer(mAAPositionAttribLocation,
                                  3, // components per attrib
                                  GL_FLOAT,
                                  GL_FALSE,
                                  0, // stride
                                  0); // offset
    mGLES2->glActiveTexture(GL_TEXTURE0);
    mGLES2->glBindTexture(GL_TEXTURE_2D, input_texture);
    mGLES2->glUniform1i(mAAInputUniformLocation, 0);
    mGLES2->glUniform2f(mAAResolutionUniformLocation, width, height);
    mGLES2->glDrawArrays(GL_TRIANGLES, 0, 3);
}
