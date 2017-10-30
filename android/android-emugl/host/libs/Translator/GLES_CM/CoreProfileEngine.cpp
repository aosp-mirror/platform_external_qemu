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

#include "CoreProfileEngineShaders.h"
#include "GLcommon/GLEScontext.h"
#include "GLcommon/GLESpointer.h"
#include "GLEScmContext.h"

#include "android/base/containers/Lookup.h"
#include "emugl/common/crash_reporter.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CoreProfileEngine::CoreProfileEngine(GLEScmContext* ctx) : mCtx(ctx) {
    mProjMatrices.resize(1, glm::mat4());
    mModelviewMatrices.resize(1, glm::mat4());
    mTextureMatrices.resize(kMaxTextureUnits, { glm::mat4() });
    mTexUnitEnvs.resize(kMaxTextureUnits, {});
    mTexGens.resize(kMaxTextureUnits, {});
    getGeometryDrawState();

    for (int i = 0; i < kMaxTextureUnits; i++) {
        mTexUnitEnvs[i][GL_TEXTURE_ENV_MODE].intVal[0] = GL_MODULATE;
        mTexUnitEnvs[i][GL_TEXTURE_ENV_COLOR].floatVal[0] = 0.2f;
        mTexUnitEnvs[i][GL_TEXTURE_ENV_COLOR].floatVal[1] = 0.4f;
        mTexUnitEnvs[i][GL_TEXTURE_ENV_COLOR].floatVal[2] = 0.8f;
        mTexUnitEnvs[i][GL_TEXTURE_ENV_COLOR].floatVal[3] = 0.7f;
        mTexUnitEnvs[i][GL_COMBINE_RGB].intVal[0] = GL_REPLACE;
        mTexUnitEnvs[i][GL_COMBINE_ALPHA].intVal[0] = GL_REPLACE;
    }
}

CoreProfileEngine::~CoreProfileEngine() {
    teardown();
}

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

        gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)0);
        gl.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                 (GLvoid*)(uintptr_t)(3 * sizeof(float)));

        gl.glBindVertexArray(0);

        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    return m_drawTexOESCoreState;
}

void CoreProfileEngine::teardown() {
    GLDispatch& gl = GLEScontext::dispatcher();

    if (m_drawTexOESCoreState.program) {
        gl.glDeleteProgram(m_drawTexOESCoreState.program);
        m_drawTexOESCoreState.program = 0;
    }

    if (m_drawTexOESCoreState.vao) {
        gl.glBindVertexArray(0);
        gl.glDeleteVertexArrays(1, &m_drawTexOESCoreState.vao);
        gl.glDeleteBuffers(1, &m_drawTexOESCoreState.ibo);
        gl.glDeleteBuffers(1, &m_drawTexOESCoreState.vbo);
        m_drawTexOESCoreState.vao = 0;
        m_drawTexOESCoreState.vbo = 0;
        m_drawTexOESCoreState.ibo = 0;
    }

    if (m_geometryDrawState.program) {
        gl.glDeleteProgram(m_geometryDrawState.program);
        m_geometryDrawState.program = 0;
    }

    if (m_geometryDrawState.programFlat) {
        gl.glDeleteProgram(m_geometryDrawState.programFlat);
        m_geometryDrawState.programFlat = 0;
    }

    if (m_geometryDrawState.vao) {
        gl.glDeleteVertexArrays(1, &m_geometryDrawState.vao);
        m_geometryDrawState.vao = 0;
    }
    if (m_geometryDrawState.posVbo) {
        gl.glDeleteBuffers(1, &m_geometryDrawState.posVbo);
        m_geometryDrawState.posVbo = 0;
    }

    if (m_geometryDrawState.normalVbo) {
        gl.glDeleteBuffers(1, &m_geometryDrawState.normalVbo);
        m_geometryDrawState.normalVbo = 0;
    }

    if (m_geometryDrawState.colorVbo) {
        gl.glDeleteBuffers(1, &m_geometryDrawState.colorVbo);
        m_geometryDrawState.colorVbo = 0;
    }

    if (m_geometryDrawState.pointsizeVbo) {
        gl.glDeleteBuffers(1, &m_geometryDrawState.pointsizeVbo);
        m_geometryDrawState.pointsizeVbo = 0;
    }

    if (m_geometryDrawState.texcoordVbo) {
        gl.glDeleteBuffers(1, &m_geometryDrawState.texcoordVbo);
        m_geometryDrawState.texcoordVbo = 0;
    }

    if (m_geometryDrawState.ibo) {
        gl.glDeleteBuffers(1, &m_geometryDrawState.ibo);
        m_geometryDrawState.ibo = 0;
    }
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

static std::string sMakeGeometryDrawShader(GLenum shaderType, bool flat) {
    // Set up a std::string to hold the result of
    // interpolating the template. We will require some extra padding
    // in the result string depending on how many characters
    // we could potentially insert.
    // For now it's just 10 chars and for a
    // single interpolation qualifier |flat|.
    static const char flatKeyword[] = "flat";

    size_t extraStringLengthRequired = 10 +
        sizeof(flatKeyword);

    size_t reservation = extraStringLengthRequired;
    std::string res;
    const char* shaderTemplate = nullptr;

    switch (shaderType) {
    case GL_VERTEX_SHADER:
        reservation += sizeof(kGeometryDrawVShaderSrcTemplate);
        shaderTemplate = kGeometryDrawVShaderSrcTemplate;
        break;
    case GL_FRAGMENT_SHADER:
        reservation += sizeof(kGeometryDrawFShaderSrcTemplate);
        shaderTemplate = kGeometryDrawFShaderSrcTemplate;
        break;
    default:
        emugl_crash_reporter(
            "%s: unknown shader type 0x%x (memory corrupt)\n", __func__,
            shaderType);
    }

    if (shaderTemplate) {
        res.resize(reservation);
        snprintf(&res[0], res.size(), shaderTemplate,
                flat ? flatKeyword : "");
    }
    return res;
}

const CoreProfileEngine::GeometryDrawState& CoreProfileEngine::getGeometryDrawState() {
    auto& gl = GLEScontext::dispatcher();

    if (!m_geometryDrawState.program) {

        // Non-flat shader
        m_geometryDrawState.vshader =
            GLEScontext::compileAndValidateCoreShader(
                GL_VERTEX_SHADER,
                sMakeGeometryDrawShader(GL_VERTEX_SHADER, false /* no flat shading */).c_str());

        m_geometryDrawState.fshader =
            GLEScontext::compileAndValidateCoreShader(
                GL_FRAGMENT_SHADER,
                sMakeGeometryDrawShader(GL_FRAGMENT_SHADER, false /* no flat shading */).c_str());

        m_geometryDrawState.program =
            GLEScontext::linkAndValidateProgram(m_geometryDrawState.vshader,
                                                m_geometryDrawState.fshader);

        m_geometryDrawState.vshaderFlat =
            GLEScontext::compileAndValidateCoreShader(
                    GL_VERTEX_SHADER,
                    sMakeGeometryDrawShader(GL_VERTEX_SHADER, true /* flat shading */).c_str());
        m_geometryDrawState.fshaderFlat =
            GLEScontext::compileAndValidateCoreShader(
                    GL_FRAGMENT_SHADER,
                    sMakeGeometryDrawShader(GL_FRAGMENT_SHADER, true /* flat shading */).c_str());
        m_geometryDrawState.programFlat =
            GLEScontext::linkAndValidateProgram(m_geometryDrawState.vshaderFlat,
                                                m_geometryDrawState.fshaderFlat);

        m_geometryDrawState.projMatrixLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "projection");
        m_geometryDrawState.modelviewMatrixLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "modelview");
        m_geometryDrawState.textureSamplerLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "tex_sampler");
        m_geometryDrawState.textureCubeSamplerLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "tex_cube_sampler");

        m_geometryDrawState.enableTextureLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_textures");
        m_geometryDrawState.enableLightingLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_lighting");
        m_geometryDrawState.enableFogLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_fog");

        m_geometryDrawState.enableReflectionMapLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_reflection_map");

        m_geometryDrawState.textureEnvModeLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "texture_env_mode");

        m_geometryDrawState.textureFormatLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "texture_format");

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

GLenum CoreProfileEngine::currTextureEnvMode() {
    return mTexUnitEnvs[mCurrTextureUnit][GL_TEXTURE_ENV_MODE].intVal[0];
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

template <class T>
static GLsizei sNeededVboCount(GLsizei indicesCount, const T* indices) {
    T maxIndex = 0;
    for (GLsizei i = 0; i < indicesCount; i++) {
        T index = indices[i];
        maxIndex = (index > maxIndex) ? index : maxIndex;
    }
    return (GLsizei)maxIndex + 1;
}

void CoreProfileEngine::setupArrayForDraw(
    GLenum arrayType,
    GLESpointer* p,
    GLint first, GLsizei count,
    bool isIndexed, GLenum indicesType, const GLvoid* indices) {

    auto& gl = GLEScontext::dispatcher();
    gl.glBindVertexArray(m_geometryDrawState.vao);

    GLint attribNum = arrayTypeToCoreAttrib(arrayType);

    GLsizei vboCount = 0;

    if (isIndexed) {

        GLsizei indexSize = 4;
        GLsizei indicesCount = count;

        switch (indicesType) {
        case GL_UNSIGNED_BYTE:
            indexSize = 1;
            vboCount = sNeededVboCount(indicesCount, (unsigned char*)indices);
            break;
        case GL_UNSIGNED_SHORT:
            indexSize = 2;
            vboCount = sNeededVboCount(indicesCount, (uint16_t*)indices);
            break;
        case GL_UNSIGNED_INT:
        default:
            indexSize = 4;
            vboCount = sNeededVboCount(indicesCount, (uint32_t*)indices);
        }

        // And send the indices
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_geometryDrawState.ibo);
        gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize * indicesCount, indices, GL_STREAM_DRAW);
    } else {
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        vboCount = count;
    }

    if (p->isEnable()) {
        gl.glEnableVertexAttribArray(attribNum);
        gl.glBindBuffer(GL_ARRAY_BUFFER, getVboFor(arrayType));

        GLESConversionArrays arrs;

        bool convert = mCtx->doConvert(arrs, first, count, indicesType, indices, !isIndexed, p, arrayType);
        ArrayData currentArr = arrs.getCurrentArray();

        GLint size = p->getSize();
        GLenum dataType = convert ? currentArr.type : p->getType();
        GLsizei stride = convert ? currentArr.stride : p->getStride();

        GLsizei effectiveStride =
            stride ? stride :
                     (size * sizeOfType(dataType));

        char* bufData = convert ? (char*)currentArr.data : (char*)p->getData();
        uint32_t offset = first * effectiveStride;
        uint32_t bufSize = offset + vboCount * effectiveStride;

        gl.glBufferData(GL_ARRAY_BUFFER, bufSize, bufData, GL_STREAM_DRAW);

        gl.glVertexAttribDivisor(attribNum, 0);
        GLboolean shouldNormalize = GL_FALSE;

        if (arrayType == GL_COLOR_ARRAY &&
            (dataType == GL_BYTE ||
             dataType == GL_UNSIGNED_BYTE ||
             dataType == GL_INT ||
             dataType == GL_UNSIGNED_INT ||
             dataType == GL_FIXED))
            shouldNormalize = true;

        gl.glVertexAttribPointer(attribNum, size, dataType,
                                 shouldNormalize ? GL_TRUE : GL_FALSE /* normalized */,
                                 effectiveStride, nullptr /* no offset into vbo */);
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
            gl.glVertexAttribPointer(attribNum, size, dataType,
                                     GL_FALSE /* normalized */,
                                     stride, nullptr /* no offset into vbo */);
            gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else {
            gl.glDisableVertexAttribArray(attribNum);
        }
    }

    gl.glBindVertexArray(0);
}

void CoreProfileEngine::bindTextureWithTextureUnitEmulation(
    GLenum target, GLuint texture, GLuint globalTexName) {
    if (target == GL_TEXTURE_CUBE_MAP) {
        mCubeMapBoundTextures[mCurrTextureUnit] = globalTexName;
    }
}

// API

void CoreProfileEngine::enable(GLenum cap) {
    mEnables.insert(cap);

    switch (cap) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
        case GL_TEXTURE_GEN_STR_OES:
            return;
        default:
            break;
    }

    auto& gl = GLEScontext::dispatcher();
    gl.glEnable(cap);
}

void CoreProfileEngine::disable(GLenum cap) {
    mEnables.erase(cap);

    switch (cap) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
        case GL_TEXTURE_GEN_STR_OES:
            return;
        default:
            break;
    }

    auto& gl = GLEScontext::dispatcher();
    gl.glDisable(cap);
}

void CoreProfileEngine::shadeModel(GLenum mode) {
    mShadeModel = mode;
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
    currMatrixStack().emplace_back(currMatrixStack().back());
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

void CoreProfileEngine::orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    currMatrix() *= glm::ortho(left, right, bottom, top, zNear, zFar);
}

void CoreProfileEngine::frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    currMatrix() *= glm::frustum(left, right, bottom, top, zNear, zFar);
}

void CoreProfileEngine::texEnvf(GLenum target, GLenum pname, GLfloat param) {
    // Assume |target| is GL_TEXTURE_ENV
    if (pname == GL_TEXTURE_ENV_MODE) {
        texEnvi(target, pname, (GLint)param);
    } else {
        mTexUnitEnvs[mCurrTextureUnit][pname].floatVal[0] = param;
    }
}

void CoreProfileEngine::texEnvfv(GLenum target, GLenum pname, const GLfloat* params) {
    if (pname == GL_TEXTURE_ENV_COLOR) {
        for (int i = 0; i < 4; i++) {
            mTexUnitEnvs[mCurrTextureUnit][pname].floatVal[i] = params[i];
        }
    } else {
        texEnvf(target, pname, params[0]);
    }
}

void CoreProfileEngine::texEnvi(GLenum target, GLenum pname, GLint param) {
    mTexUnitEnvs[mCurrTextureUnit][pname].intVal[0] = param;
}

void CoreProfileEngine::texEnviv(GLenum target, GLenum pname, const GLint* params) {
    mTexUnitEnvs[mCurrTextureUnit][pname].intVal[0] = params[0];
}

void CoreProfileEngine::getTexEnvfv(GLenum env, GLenum pname, GLfloat* params) {
    *params = mTexUnitEnvs[mCurrTextureUnit][pname].floatVal[0];
}

void CoreProfileEngine::getTexEnviv(GLenum env, GLenum pname, GLint* params) {
    *params = mTexUnitEnvs[mCurrTextureUnit][pname].intVal[0];
}

void CoreProfileEngine::texGenf(GLenum coord, GLenum pname, GLfloat param) {
    mTexGens[mCurrTextureUnit][pname].floatVal[0] = param;
}

void CoreProfileEngine::texGenfv(GLenum coord, GLenum pname, const GLfloat* params) {
    mTexGens[mCurrTextureUnit][pname].floatVal[0] = params[0];
}

void CoreProfileEngine::texGeni(GLenum coord, GLenum pname, GLint param) {
    mTexGens[mCurrTextureUnit][pname].intVal[0] = param;
}

void CoreProfileEngine::texGeniv(GLenum coord, GLenum pname, const GLint* params) {
    mTexGens[mCurrTextureUnit][pname].intVal[0] = params[0];
}

void CoreProfileEngine::getTexGeniv(GLenum coord, GLenum pname, GLint* params) {
    *params = mTexGens[mCurrTextureUnit][pname].intVal[0];
}

void CoreProfileEngine::getTexGenfv(GLenum coord, GLenum pname, GLfloat* params) {
    params[0] = mTexGens[mCurrTextureUnit][pname].floatVal[0];
    params[1] = mTexGens[mCurrTextureUnit][pname].floatVal[1];
    params[2] = mTexGens[mCurrTextureUnit][pname].floatVal[2];
    params[3] = mTexGens[mCurrTextureUnit][pname].floatVal[3];
}

void CoreProfileEngine::enableClientState(GLenum clientState) {

}

void CoreProfileEngine::disableClientState(GLenum clientState) {
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
    z = z >= 1.0f ? 1.0f : z;
    z = z <= 0.0f ? 0.0f : z;
    float zNdc = z * 2.0f - 1.0f;

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
                    xNdc, yNdc, zNdc,
                    texCropX / texW, texCropY / texH,

                    xNdc + wNdc, yNdc, zNdc,
                    (texCropX + texCropW) / texW, texCropY / texH,

                    xNdc + wNdc, yNdc + hNdc, zNdc,
                    (texCropX + texCropW) / texW, (texCropY + texCropH) / texH,

                    xNdc, yNdc + hNdc, zNdc,
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

void CoreProfileEngine::scalef(float x, float y, float z) {
    glm::mat4 scale = glm::scale(glm::mat4(), glm::vec3(x, y, z));
    currMatrix() *= scale;
}

void CoreProfileEngine::translatef(float x, float y, float z) {
    glm::mat4 tr = glm::translate(glm::mat4(), glm::vec3(x, y, z));
    currMatrix() *= tr;
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
    // Use 2 texture image units for each texture unit of GLES1.
    // This allows us to simultaneously use GL_TEXTURE_2D
    // and GL_TEXTURE_CUBE_MAP with the same texture unit through
    // using different samplers for the image units.
    GLEScontext::dispatcher().glActiveTexture(GL_TEXTURE0 + mCurrTextureUnit * 2);
}

void CoreProfileEngine::clientActiveTexture(GLenum unit) {
    mCurrTextureUnit = unit - GL_TEXTURE0;
    GLEScontext::dispatcher().glActiveTexture(unit);
}

glm::mat4 CoreProfileEngine::getProjMatrix() const {
    return mProjMatrices.back();
}

glm::mat4 CoreProfileEngine::getModelviewMatrix() const {
    return mModelviewMatrices.back();
}

void CoreProfileEngine::preDrawTextureUnitEmulation() {
    auto& gl = GLEScontext::dispatcher();

    gl.glUniform1i(m_geometryDrawState.enableTextureLoc,
                   isEnabled(GL_TEXTURE_2D) &&
                   mCtx->isArrEnabled(GL_TEXTURE_COORD_ARRAY));
    gl.glUniform1i(m_geometryDrawState.textureSamplerLoc, mCurrTextureUnit * 2);
    gl.glUniform1i(m_geometryDrawState.textureCubeSamplerLoc, mCurrTextureUnit * 2 + 1);

    if (auto cubeMapTex = android::base::find(mCubeMapBoundTextures, mCurrTextureUnit)) {
        if (*cubeMapTex) {
            GLuint cubeMapTexId = *cubeMapTex;
            gl.glActiveTexture(GL_TEXTURE0 + mCurrTextureUnit * 2);
            gl.glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            gl.glActiveTexture(GL_TEXTURE0 + mCurrTextureUnit * 2 + 1);
            gl.glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexId);
            gl.glActiveTexture(GL_TEXTURE0 + mCurrTextureUnit * 2);
        }
    }

    if (auto genmode = android::base::find(mTexGens[mCurrTextureUnit], GL_TEXTURE_GEN_MODE_OES)) {
        if (genmode->intVal[0] == GL_REFLECTION_MAP_OES) {
            gl.glUniform1i(m_geometryDrawState.enableTextureLoc, 1);
            gl.glUniform1i(m_geometryDrawState.enableReflectionMapLoc, 1);
        }
    }

    auto bindedTex = mCtx->getBindedTexture(GL_TEXTURE_2D);
    ObjectLocalName tex = mCtx->getTextureLocalName(GL_TEXTURE_2D, bindedTex);
    auto objData = mCtx->shareGroup()->getObjectData(NamedObjectType::TEXTURE, tex);

    if (objData) {
        TextureData* texData = (TextureData*)objData;
        gl.glUniform1i(m_geometryDrawState.textureFormatLoc, texData->internalFormat);
    } else {
        gl.glUniform1i(m_geometryDrawState.textureFormatLoc, GL_RGBA);
    }

    gl.glUniform1i(m_geometryDrawState.textureEnvModeLoc, currTextureEnvMode());
}

void CoreProfileEngine::postDrawTextureUnitEmulation() {
    auto& gl = GLEScontext::dispatcher();

    GLuint cubeMapTexId = 0;

    if (auto cubeMapTex = android::base::find(mCubeMapBoundTextures, mCurrTextureUnit)) {
        cubeMapTexId = *cubeMapTex;
    }

    if (cubeMapTexId) {
        gl.glActiveTexture(GL_TEXTURE0 + mCurrTextureUnit * 2);
        gl.glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexId);
        gl.glActiveTexture(GL_TEXTURE0 + mCurrTextureUnit * 2 + 1);
        gl.glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        gl.glActiveTexture(GL_TEXTURE0 + mCurrTextureUnit * 2);
    }
}

void CoreProfileEngine::preDrawVertexSetup() {
    auto& gl = GLEScontext::dispatcher();

    glm::mat4 currProjMatrix = getProjMatrix();
    glm::mat4 currModelviewMatrix = getModelviewMatrix();

    gl.glBindVertexArray(m_geometryDrawState.vao);
    gl.glUseProgram(mShadeModel == GL_FLAT ?
                    m_geometryDrawState.programFlat :
                    m_geometryDrawState.program);
    gl.glUniformMatrix4fv(m_geometryDrawState.projMatrixLoc, 1, GL_FALSE, glm::value_ptr(currProjMatrix));
    gl.glUniformMatrix4fv(m_geometryDrawState.modelviewMatrixLoc, 1, GL_FALSE, glm::value_ptr(currModelviewMatrix));
}

void CoreProfileEngine::postDrawVertexSetup() {
    auto& gl = GLEScontext::dispatcher();

    gl.glBindVertexArray(0);
}

void CoreProfileEngine::drawArrays(GLenum type, GLint first, GLsizei count) {
    auto& gl = GLEScontext::dispatcher();

    preDrawVertexSetup();
    preDrawTextureUnitEmulation();

    gl.glDrawArrays(type, first, count);

    postDrawVertexSetup();
    postDrawTextureUnitEmulation();
}

void CoreProfileEngine::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    auto& gl = GLEScontext::dispatcher();

    preDrawVertexSetup();
    preDrawTextureUnitEmulation();

    gl.glDrawElements(mode, count, type, (GLvoid*)0);

    postDrawVertexSetup();
    postDrawTextureUnitEmulation();
}
