/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License")
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "GLcommon/ScopedGLState.h"

#include "GLcommon/GLEScontext.h"

#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl31.h>

#include <unordered_map>

void ScopedGLState::push(GLenum name) {
    auto& gl = GLEScontext::dispatcher();

    StateVector v;

    switch (name) {
    case GL_DRAW_FRAMEBUFFER_BINDING:
    case GL_READ_FRAMEBUFFER_BINDING:
    case GL_CURRENT_PROGRAM:
    case GL_VERTEX_ARRAY_BINDING:
    case GL_ARRAY_BUFFER_BINDING:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_CUBE_MAP:
    case GL_VIEWPORT:
    case GL_COLOR_WRITEMASK:
        gl.glGetIntegerv(name, v.intData);
        break;
    case GL_DEPTH_RANGE:
        gl.glGetFloatv(name, v.floatData);
        break;
    // glIsEnabled queries
    case GL_BLEND:
    case GL_SCISSOR_TEST:
    case GL_DEPTH_TEST:
    case GL_STENCIL_TEST:
    case GL_CULL_FACE:
    case GL_RASTERIZER_DISCARD:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_POLYGON_OFFSET_FILL:
        v.intData[0] = gl.glIsEnabled(name);
        break;
    default:
        fprintf(stderr,
                "%s: ScopedGLState doesn't support 0x%x yet, it's mainly for "
                "texture emulation by drawing fullscreen quads.\n", __func__,
                name);
        break;
    }

    mStateMap[name] = v;
}

void ScopedGLState::push(std::initializer_list<GLenum> names) {
    for (const auto name : names) {
        push(name);
    }
}

void ScopedGLState::pushForCoreProfileTextureEmulation() {
    push({ GL_DRAW_FRAMEBUFFER_BINDING,
           GL_READ_FRAMEBUFFER_BINDING,
           GL_VERTEX_ARRAY_BINDING,
           GL_CURRENT_PROGRAM,
           GL_VIEWPORT,
           GL_SCISSOR_TEST,
           GL_DEPTH_TEST,
           GL_BLEND,
           GL_DEPTH_RANGE,
           GL_COLOR_WRITEMASK,
           GL_SAMPLE_ALPHA_TO_COVERAGE,
           GL_SAMPLE_COVERAGE,
           GL_CULL_FACE,
           GL_POLYGON_OFFSET_FILL,
           GL_RASTERIZER_DISCARD,
           GL_TEXTURE_BINDING_2D });
}

ScopedGLState::~ScopedGLState() {
    auto& gl = GLEScontext::dispatcher();

    for (const auto it : mStateMap) {
        GLenum name = it.first;
        const StateVector& v = it.second;
        switch (name) {
            case GL_DRAW_FRAMEBUFFER_BINDING:
                gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, v.intData[0]);
            case GL_READ_FRAMEBUFFER_BINDING:
                gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, v.intData[0]);
                break;
            case GL_CURRENT_PROGRAM:
                gl.glUseProgram(v.intData[0]);
                break;
            case GL_VERTEX_ARRAY_BINDING:
                gl.glBindVertexArray(v.intData[0]);
                break;
            case GL_ARRAY_BUFFER_BINDING:
                gl.glBindBuffer(GL_ARRAY_BUFFER, v.intData[0]);
                break;
            case GL_TEXTURE_BINDING_2D:
                gl.glBindTexture(GL_TEXTURE_2D, v.intData[0]);
                break;
            case GL_TEXTURE_BINDING_CUBE_MAP:
                gl.glBindTexture(GL_TEXTURE_CUBE_MAP, v.intData[0]);
                break;
            case GL_VIEWPORT:
                gl.glViewport(v.intData[0], v.intData[1], v.intData[2], v.intData[3]);
                break;
            case GL_COLOR_WRITEMASK:
                gl.glColorMask(v.intData[0], v.intData[1], v.intData[2], v.intData[3]);
                break;
            case GL_DEPTH_RANGE:
                gl.glDepthRange(v.floatData[0], v.floatData[1]);
                break;
                // glIsEnabled queries
            case GL_BLEND:
            case GL_SCISSOR_TEST:
            case GL_DEPTH_TEST:
            case GL_STENCIL_TEST:
            case GL_CULL_FACE:
            case GL_RASTERIZER_DISCARD:
            case GL_SAMPLE_ALPHA_TO_COVERAGE:
            case GL_SAMPLE_COVERAGE:
            case GL_POLYGON_OFFSET_FILL:
                if (v.intData[0]) {
                    gl.glEnable(name);
                } else {
                    gl.glDisable(name);
                }
                break;
            default:
                fprintf(stderr,
                        "%s: ScopedGLState doesn't support 0x%x yet, it's mainly for "
                        "texture emulation by drawing fullscreen quads.\n", __func__,
                        name);
                break;
        }
    }
}
