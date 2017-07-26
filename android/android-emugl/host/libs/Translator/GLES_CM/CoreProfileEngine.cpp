/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
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

#include "CoreProfileEngine.h"

#include "GLcommon/GLEScontext.h"
#include "GLcommon/GLESpointer.h"
#include "GLEScmContext.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CoreProfileEngine::CoreProfileEngine(GLEScmContext* ctx) : mCtx(ctx) {
    mProjMatrices.resize(1, glm::mat4());
    mModelviewMatrices.resize(1, glm::mat4());
    for (GLint i = 0; i < kMaxTextureUnits; i++) {
        mTextureMatrices.emplace_back();
        mTextureMatrices[i].resize(1, glm::mat4());
        mTexUnitEnvs.emplace_back();
    }
    getGeometryDrawState();
}

#include <CoreProfileEngineShaders.cpp>

static const uint32_t sDrawTexIbo[] = {
    0, 1, 2, 0, 2, 3,
};

const CoreProfileEngine::DrawTexOESCoreState& CoreProfileEngine::getDrawTexOESCoreState() {
    if (!m_drawTexOESCoreState.program) {
        m_drawTexOESCoreState.vshader =
            GLEScontext::compileAndValidateCoreShader(GL_VERTEX_SHADER, kDrawTexOESCore_vshader);
        m_drawTexOESCoreState.fshader =
            GLEScontext::compileAndValidateCoreShader(GL_FRAGMENT_SHADER, kDrawTexOESCore_fshader);
        m_drawTexOESCoreState.program =
            GLEScontext::linkAndValidateProgram(m_drawTexOESCoreState.vshader,
                                                m_drawTexOESCoreState.fshader);
    }
    if (!m_drawTexOESCoreState.vao) {
        GLDispatch& gl = GLEScontext::dispatcher();

        gl.glGenVertexArrays(1, &m_drawTexOESCoreState.vao);
        gl.glBindVertexArray(m_drawTexOESCoreState.vao);

        // Initialize VBO
        // Save IBO, attrib arrays/pointers to VAO
        gl.glGenBuffers(1, &m_drawTexOESCoreState.ibo);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_drawTexOESCoreState.ibo);
        gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sDrawTexIbo), sDrawTexIbo, GL_STATIC_DRAW);

        gl.glGenBuffers(1, &m_drawTexOESCoreState.vbo);
        gl.glBindBuffer(GL_ARRAY_BUFFER, m_drawTexOESCoreState.vbo);

        gl.glEnableVertexAttribArray(0); // pos
        gl.glEnableVertexAttribArray(1); // texcoord

        gl.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0);
        gl.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                 (GLvoid*)(uintptr_t)(2 * sizeof(float)));

        gl.glBindVertexArray(0);

        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    return m_drawTexOESCoreState;
}

// Match attribute locations in the shader below.
static GLint arrayTypeToCoreAttrib(GLenum type) {
    switch (type) {
    case GL_VERTEX_ARRAY:
        return 0;
    case GL_NORMAL_ARRAY:
        return 1;
    case GL_COLOR_ARRAY:
        return 2;
    case GL_POINT_SIZE_ARRAY_OES:
        return 3;
    case GL_TEXTURE_COORD_ARRAY:
        return 4;
    }
    fprintf(stderr, "%s: error: unsupported array type 0x%x\n",
            __func__, type);
    return 0;
}

const CoreProfileEngine::GeometryDrawState& CoreProfileEngine::getGeometryDrawState() {
    auto& gl = GLEScontext::dispatcher();

    if (!m_geometryDrawState.program) {
        m_geometryDrawState.vshader =
            GLEScontext::compileAndValidateCoreShader(GL_VERTEX_SHADER, kGeometryDrawVShaderSrc);
        m_geometryDrawState.fshader =
            GLEScontext::compileAndValidateCoreShader(GL_FRAGMENT_SHADER, kGeometryDrawFShaderSrc);
        m_geometryDrawState.program =
            GLEScontext::linkAndValidateProgram(m_geometryDrawState.vshader,
                                                m_geometryDrawState.fshader);

        m_geometryDrawState.projMatrixLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "projection");
        m_geometryDrawState.modelviewMatrixLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "modelview");
        m_geometryDrawState.textureSamplerLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "tex_sampler");

        m_geometryDrawState.enableTextureLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_textures");
        m_geometryDrawState.enableLightingLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_lighting");
        m_geometryDrawState.enableFogLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_fog");

        fprintf(stderr, "%s: uniform locs: %d %d %d %d %d %d\n", __func__,
                m_geometryDrawState.projMatrixLoc,
                m_geometryDrawState.modelviewMatrixLoc,
                m_geometryDrawState.textureSamplerLoc,
                m_geometryDrawState.enableTextureLoc,
                m_geometryDrawState.enableLightingLoc,
                m_geometryDrawState.enableFogLoc);
    }

    if (!m_geometryDrawState.vao) {

        gl.glGenBuffers(1, &m_geometryDrawState.posVbo);
        gl.glGenBuffers(1, &m_geometryDrawState.normalVbo);
        gl.glGenBuffers(1, &m_geometryDrawState.colorVbo);
        gl.glGenBuffers(1, &m_geometryDrawState.pointsizeVbo);
        gl.glGenBuffers(1, &m_geometryDrawState.texcoordVbo);

        gl.glGenVertexArrays(1, &m_geometryDrawState.vao);
        gl.glBindVertexArray(m_geometryDrawState.vao);

        gl.glGenBuffers(1, &m_geometryDrawState.ibo);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_geometryDrawState.ibo);

        gl.glBindVertexArray(0);

        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    return m_geometryDrawState;
}

glm::mat4& CoreProfileEngine::currMatrix() {
    return currMatrixStack().back();
}

CoreProfileEngine::MatrixStack& CoreProfileEngine::currMatrixStack() {
    switch (mCurrMatrixMode) {
    case GL_TEXTURE:
        return mTextureMatrices[mCurrTextureUnit];
    case GL_PROJECTION:
        return mProjMatrices;
    case GL_MODELVIEW:
        return mModelviewMatrices;
    default:
        fprintf(stderr, "%s: error: matrix mode set to 0x%x!\n", __func__,
                mCurrMatrixMode);
        abort();
    }
    abort();
}

GLuint CoreProfileEngine::getVboFor(GLenum type) {
    switch (type) {
    case GL_VERTEX_ARRAY:
        return m_geometryDrawState.posVbo;
    case GL_NORMAL_ARRAY:
        return m_geometryDrawState.normalVbo;
    case GL_COLOR_ARRAY:
        return m_geometryDrawState.colorVbo;
    case GL_POINT_SIZE_ARRAY_OES:
        return m_geometryDrawState.pointsizeVbo;
    case GL_TEXTURE_COORD_ARRAY:
        return m_geometryDrawState.texcoordVbo;
    }
    return 0;
}

size_t CoreProfileEngine::sizeOfType(GLenum type) {
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT_OES:
        return 2;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        return 4;
    }
    return 4;
}

void CoreProfileEngine::setupArrayForDraw(GLenum arrayType, GLESpointer* p, GLint first, GLsizei count) {
    auto& gl = GLEScontext::dispatcher();
    gl.glBindVertexArray(m_geometryDrawState.vao);

    GLint attribNum = arrayTypeToCoreAttrib(arrayType);

    if (p->isEnable()) {
        gl.glEnableVertexAttribArray(attribNum);
        gl.glBindBuffer(GL_ARRAY_BUFFER, getVboFor(arrayType));

        GLint size = p->getSize();
        GLenum dataType = p->getType();
        GLsizei stride = p->getStride();

        GLsizei effectiveStride =
            stride ? (stride * sizeOfType(dataType)) :
                     (size * sizeOfType(dataType));

        char* bufData = (char*)p->getData();
        uint32_t offset = first * effectiveStride;
        uint32_t bufSize = offset + count * effectiveStride;

        gl.glBufferData(GL_ARRAY_BUFFER, bufSize, bufData, GL_STREAM_DRAW);

        gl.glVertexAttribDivisor(attribNum, 0);
        gl.glVertexAttribPointer(attribNum, size, dataType, GL_FALSE /* normalized */, effectiveStride, nullptr);

        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);

    } else {
        if (arrayType == GL_COLOR_ARRAY) {
            gl.glEnableVertexAttribArray(attribNum);
            gl.glBindBuffer(GL_ARRAY_BUFFER, getVboFor(arrayType));

            // Use vertex attrib divisor to avoid creating a buffer
            // that repeats the same value for as many verts.

            GLint size = 4;
            GLenum dataType = GL_FLOAT;
            GLsizei stride = 4 * sizeof(float);
            std::vector<float> data(4);
            for (size_t i = 0; i < data.size(); i++) {
                data[i] = mColor.val.floatVal[i];
            }
            gl.glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), (GLvoid*)&data[0], GL_STREAM_DRAW);
            gl.glVertexAttribDivisor(attribNum, 1);
            gl.glVertexAttribPointer(attribNum, size, dataType, GL_FALSE /* normalized */, stride, nullptr);

            gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else {
            gl.glDisableVertexAttribArray(attribNum);
        }
    }

    gl.glBindVertexArray(0);
}

// API

void CoreProfileEngine::enable(GLenum cap) {
    mEnables[cap] = true;

    switch (cap) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
            return;
        default:
            break;
    }

    auto& gl = GLEScontext::dispatcher();
    gl.glEnable(cap);
}

void CoreProfileEngine::disable(GLenum cap) {
    mEnables[cap] = false;

    switch (cap) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
            return;
        default:
            break;
    }

    auto& gl = GLEScontext::dispatcher();
    gl.glDisable(cap);
}

void CoreProfileEngine::matrixMode(GLenum mode) {
    mCurrMatrixMode = mode;
}

void CoreProfileEngine::loadIdentity() {
    currMatrix() = glm::mat4();
}

void CoreProfileEngine::pushMatrix() {
    if (currMatrixStack().size() >= kMaxMatrixStackSize) {
        setError(GL_STACK_OVERFLOW);
        return;
    }
    currMatrixStack().emplace_back();
}

void CoreProfileEngine::popMatrix() {
    if (currMatrixStack().size() == 1) {
        setError(GL_STACK_UNDERFLOW);
        return;
    }
    currMatrixStack().pop_back();
}

void CoreProfileEngine::multMatrixf(const GLfloat* m) {
    currMatrix() *= glm::make_mat4(m);
}


void CoreProfileEngine::frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    currMatrix() *= glm::frustum(left, right, bottom, top, zNear, zFar);
}

void CoreProfileEngine::texEnvf(GLenum target, GLenum pname, GLfloat param) {
    mTexUnitEnvs[mCurrTextureUnit][pname].floatVal[0] = param;
}

void CoreProfileEngine::texEnvfv(GLenum target, GLenum pname, const GLfloat* params) {
    if (pname == GL_TEXTURE_ENV_COLOR) {
        for (int i = 0; i < 4; i++) {
            mTexUnitEnvs[mCurrTextureUnit][pname].floatVal[i] = params[i];
        }
    } else {
        mTexUnitEnvs[mCurrTextureUnit][pname].floatVal[0] = params[0];
    }
}

void CoreProfileEngine::drawTexOES(float x, float y, float z, float width, float height) {
    auto& gl = GLEScontext::dispatcher();

    // get viewport
    GLint viewport[4] = {};
    gl.glGetIntegerv(GL_VIEWPORT,viewport);

    // track previous vbo/ibo
    GLuint prev_vbo;
    GLuint prev_ibo;
    gl.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&prev_vbo);
    gl.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint*)&prev_ibo);

    // compile shaders, generate vbo/ibo if not done already
    CoreProfileEngine::DrawTexOESCoreState drawTexState =
        getDrawTexOESCoreState();

    GLuint prog = drawTexState.program;
    GLuint vbo = drawTexState.vbo;
    GLuint vao = drawTexState.vao;

    gl.glUseProgram(prog);

    gl.glBindVertexArray(vao);

    GLint samplerLoc = gl.glGetUniformLocation(prog, "tex_sampler");

    // Compute screen coordinates for our texture.
    // Recenter, rescale. (e.g., [0, 0, 1080, 1920] -> [-1, -1, 1, 1])
    float xNdc = 2.0f * (float)(x - viewport[0] - viewport[2] / 2) / (float)viewport[2];
    float yNdc = 2.0f * (float)(y - viewport[1] - viewport[3] / 2) / (float)viewport[3];
    float wNdc = 2.0f * (float)width / (float)viewport[2];
    float hNdc = 2.0f * (float)height / (float)viewport[3];

    for (int i = 0; i < mCtx->getMaxTexUnits(); i++) {
        if (mCtx->isTextureUnitEnabled(GL_TEXTURE0 + i)) {
            GLuint bindedTex = mCtx->getBindedTexture(GL_TEXTURE0 + i, GL_TEXTURE_2D);
            ObjectLocalName tex = mCtx->getTextureLocalName(GL_TEXTURE_2D, bindedTex);

            auto objData = mCtx->shareGroup()->getObjectData(NamedObjectType::TEXTURE, tex);

            if (objData) {
                TextureData* texData = (TextureData*)objData;

                float texCropX = (float)(texData->crop_rect[0]);
                float texCropY = (float)(texData->crop_rect[1]);

                float texCropW = (float)(texData->crop_rect[2]);
                float texCropH = (float)(texData->crop_rect[3]);

                float texW = (float)(texData->width);
                float texH = (float)(texData->height);

                // Now we know the vertex attributes (pos, texcoord).
                // Our vertex attributes are formatted with interleaved
                // position and texture coordinate:
                float vertexAttrs[] = {
                    xNdc, yNdc,
                    texCropX / texW, texCropY / texH,

                    xNdc + wNdc, yNdc,
                    (texCropX + texCropW) / texW, texCropY / texH,

                    xNdc + wNdc, yNdc + hNdc,
                    (texCropX + texCropW) / texW, (texCropY + texCropH) / texH,

                    xNdc, yNdc + hNdc,
                    texCropX / texW, (texCropY + texCropH) / texH,
                };

                gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);
                gl.glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrs),
                                vertexAttrs, GL_STREAM_DRAW);
            }

            gl.glActiveTexture(GL_TEXTURE0 + i);
            gl.glUniform1i(samplerLoc, i);
            gl.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }

    gl.glBindVertexArray(0);
    gl.glUseProgram(0);

    gl.glBindBuffer(GL_ARRAY_BUFFER, prev_vbo);
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prev_ibo);
}

void CoreProfileEngine::rotatef(float angle, float x, float y, float z) {
    glm::mat4 rot = glm::rotate(glm::mat4(), 3.14159265358979f / 180.0f * angle, glm::vec3(x, y, z));

    currMatrix() *= rot;
}

void CoreProfileEngine::color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    mColor.type = GL_FLOAT;
    mColor.val.floatVal[0] = red;
    mColor.val.floatVal[1] = green;
    mColor.val.floatVal[2] = blue;
    mColor.val.floatVal[3] = alpha;
}

void CoreProfileEngine::color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    mColor.type = GL_UNSIGNED_BYTE;
    mColor.val.ubyteVal[0] = red;
    mColor.val.ubyteVal[1] = green;
    mColor.val.ubyteVal[2] = blue;
    mColor.val.ubyteVal[3] = alpha;
}

void CoreProfileEngine::activeTexture(GLenum unit) {
    mCurrTextureUnit = unit - GL_TEXTURE0;
    GLEScontext::dispatcher().glActiveTexture(unit);
}

void CoreProfileEngine::clientActiveTexture(GLenum unit) {
    mCurrTextureUnit = unit - GL_TEXTURE0;
    GLEScontext::dispatcher().glActiveTexture(unit);
}

glm::mat4 CoreProfileEngine::getProjMatrix() const {
    glm::mat4 res = mProjMatrices[0];
    for (size_t i = 1; i < mProjMatrices.size(); i++) {
        res *= mProjMatrices[i];
    }
    return res;
}

glm::mat4 CoreProfileEngine::getModelviewMatrix() const {
    glm::mat4 res = mModelviewMatrices[0];
    for (size_t i = 1; i < mModelviewMatrices.size(); i++) {
        res *= mModelviewMatrices[i];
    }
    return res;
}

void CoreProfileEngine::drawArrays(GLenum type, GLint first, GLsizei count) {
    auto& gl = GLEScontext::dispatcher();

    fprintf(stderr, "%s: type 0x%x first %d count %d\n", __func__, type, first, count);

    glm::mat4 currProjMatrix = getProjMatrix();
    glm::mat4 currModelviewMatrix = getModelviewMatrix();

    gl.glBindVertexArray(m_geometryDrawState.vao);
    gl.glUseProgram(m_geometryDrawState.program);
    gl.glUniformMatrix4fv(m_geometryDrawState.projMatrixLoc, 1, GL_FALSE, glm::value_ptr(currProjMatrix));
    gl.glUniformMatrix4fv(m_geometryDrawState.modelviewMatrixLoc, 1, GL_FALSE, glm::value_ptr(currModelviewMatrix));

    gl.glUniform1i(m_geometryDrawState.enableTextureLoc,
                   mEnables[GL_TEXTURE_2D]);
    gl.glUniform1i(m_geometryDrawState.textureSamplerLoc, 0);

    // gl.glUniform1i(m_geometryDrawState.enableLightingLoc, 1); debug
    gl.glDrawArrays(type, first, count);

    gl.glBindVertexArray(0);
}
