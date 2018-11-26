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
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CoreProfileEngine::CoreProfileEngine(GLEScmContext* ctx, bool onGles) :
    mCtx(ctx),
    mOnGles(onGles) {
    getGeometryDrawState();
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
            GLEScontext::compileAndValidateCoreShader(
                GL_VERTEX_SHADER,
                mOnGles ? kDrawTexOESGles2_vshader : kDrawTexOESCore_vshader);
        m_drawTexOESCoreState.fshader =
            GLEScontext::compileAndValidateCoreShader(
                GL_FRAGMENT_SHADER,
                mOnGles ? kDrawTexOESGles2_fshader : kDrawTexOESCore_fshader);
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

static std::string sMakeGeometryDrawShader(bool isGles, GLenum shaderType, bool flat) {
    // Set up a std::string to hold the result of
    // interpolating the template. We will require some extra padding
    // in the result string depending on how many characters
    // we could potentially insert.
    // For now it's just 10 chars and for a
    // single interpolation qualifier |flat|.
    static const char versionPartEssl300[] = "#version 300 es\n";
    static const char versionPart330Core[] = "#version 330 core\n";
    static const char flatKeyword[] = "flat";

    size_t extraStringLengthRequired = 10 +
        sizeof(versionPartEssl300) +
        sizeof(versionPart330Core) +
        sizeof(flatKeyword);

    size_t reservation = extraStringLengthRequired;
    std::string res;
    const char* shaderTemplate = nullptr;

    switch (shaderType) {
    case GL_VERTEX_SHADER:
        reservation += sizeof(kGeometryDrawVShaderSrcTemplateCore);
        shaderTemplate = kGeometryDrawVShaderSrcTemplateCore;
        break;
    case GL_FRAGMENT_SHADER:
        reservation += sizeof(kGeometryDrawFShaderSrcTemplateCore);
        shaderTemplate = kGeometryDrawFShaderSrcTemplateCore;
        break;
    default:
        emugl::emugl_crash_reporter(
                "%s: unknown shader type 0x%x (memory corrupt)\n", __func__,
                shaderType);
    }

    if (shaderTemplate) {
        res.resize(reservation);
        snprintf(&res[0], res.size(), shaderTemplate,
                isGles ? versionPartEssl300 : versionPart330Core,
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
                sMakeGeometryDrawShader(mOnGles, GL_VERTEX_SHADER, false /* no flat shading */).c_str());

        m_geometryDrawState.fshader =
            GLEScontext::compileAndValidateCoreShader(
                GL_FRAGMENT_SHADER,
                sMakeGeometryDrawShader(mOnGles, GL_FRAGMENT_SHADER, false /* no flat shading */).c_str());

        m_geometryDrawState.program =
            GLEScontext::linkAndValidateProgram(m_geometryDrawState.vshader,
                                                m_geometryDrawState.fshader);

        m_geometryDrawState.vshaderFlat =
            GLEScontext::compileAndValidateCoreShader(
                    GL_VERTEX_SHADER,
                    sMakeGeometryDrawShader(mOnGles, GL_VERTEX_SHADER, true /* flat shading */).c_str());
        m_geometryDrawState.fshaderFlat =
            GLEScontext::compileAndValidateCoreShader(
                    GL_FRAGMENT_SHADER,
                    sMakeGeometryDrawShader(mOnGles, GL_FRAGMENT_SHADER, true /* flat shading */).c_str());
        m_geometryDrawState.programFlat =
            GLEScontext::linkAndValidateProgram(m_geometryDrawState.vshaderFlat,
                                                m_geometryDrawState.fshaderFlat);

        m_geometryDrawState.projMatrixLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "projection");
        m_geometryDrawState.modelviewMatrixLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "modelview");
        m_geometryDrawState.modelviewInvTrLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "modelview_invtr");
        m_geometryDrawState.textureMatrixLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "texture_matrix");
        m_geometryDrawState.textureSamplerLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "tex_sampler");
        m_geometryDrawState.textureCubeSamplerLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "tex_cube_sampler");

        m_geometryDrawState.enableTextureLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_textures");
        m_geometryDrawState.enableLightingLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_lighting");
        m_geometryDrawState.enableRescaleNormalLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_rescale_normal");
        m_geometryDrawState.enableNormalizeLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_normalize");
        m_geometryDrawState.enableColorMaterialLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_color_material");
        m_geometryDrawState.enableFogLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_fog");
        m_geometryDrawState.enableReflectionMapLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "enable_reflection_map");

        m_geometryDrawState.textureEnvModeLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "texture_env_mode");
        m_geometryDrawState.textureFormatLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "texture_format");

        m_geometryDrawState.materialAmbientLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "material_ambient");
        m_geometryDrawState.materialDiffuseLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "material_diffuse");
        m_geometryDrawState.materialSpecularLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "material_specular");
        m_geometryDrawState.materialEmissiveLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "material_emissive");
        m_geometryDrawState.materialSpecularExponentLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "material_specular_exponent");

        m_geometryDrawState.lightModelSceneAmbientLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_model_scene_ambient");
        m_geometryDrawState.lightModelTwoSidedLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_model_two_sided");

        m_geometryDrawState.lightEnablesLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_enables");
        m_geometryDrawState.lightAmbientsLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_ambients");
        m_geometryDrawState.lightDiffusesLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_diffuses");
        m_geometryDrawState.lightSpecularsLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_speculars");
        m_geometryDrawState.lightPositionsLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_positions");
        m_geometryDrawState.lightDirectionsLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_directions");
        m_geometryDrawState.lightSpotlightExponentsLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_spotlight_exponents");
        m_geometryDrawState.lightSpotlightCutoffAnglesLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_spotlight_cutoff_angles");
        m_geometryDrawState.lightAttenuationConstsLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_attenuation_consts");
        m_geometryDrawState.lightAttenuationLinearsLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_attenuation_linears");
        m_geometryDrawState.lightAttenuationQuadraticsLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "light_attenuation_quadratics");

        m_geometryDrawState.fogModeLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "fog_mode");
        m_geometryDrawState.fogDensityLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "fog_density");
        m_geometryDrawState.fogStartLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "fog_start");
        m_geometryDrawState.fogEndLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "fog_end");
        m_geometryDrawState.fogColorLoc =
            gl.glGetUniformLocation(m_geometryDrawState.program, "fog_color");

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
    GLint attribNum = -1;

    gl.glBindVertexArray(m_geometryDrawState.vao);

    attribNum = arrayTypeToCoreAttrib(arrayType);

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
        GLboolean shouldNormalize = false;

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
        if (arrayType == GL_COLOR_ARRAY ||
            arrayType == GL_NORMAL_ARRAY ||
            arrayType == GL_TEXTURE_COORD_ARRAY) {

            gl.glEnableVertexAttribArray(attribNum);
            gl.glBindBuffer(GL_ARRAY_BUFFER, getVboFor(arrayType));

            // Use vertex attrib divisor to avoid creating a buffer
            // that repeats the same value for as many verts.

            GLint size = 4;
            GLenum dataType = GL_FLOAT;
            std::vector<float> data(4, 0.f);
            switch (arrayType) {
                case GL_VERTEX_ARRAY:
                    size = 4;
                    fprintf(stderr, "Error: GLES1 does not support immediate vertices\n");
                    mCtx->setGLerror(GL_INVALID_OPERATION);
                    break;
                case GL_COLOR_ARRAY:
                    size = 4;
                    data = mCtx->getColor();
                    break;
                case GL_NORMAL_ARRAY:
                    size = 3;
                    data = mCtx->getNormal();
                    break;
                case GL_POINT_SIZE_ARRAY_OES:
                    data = mCtx->getColor();
                    break;
                case GL_TEXTURE_COORD_ARRAY:
                    size = 4;
                    data = mCtx->getMultiTexCoord(mCtx->getActiveTextureUnit());
                    break;
                default:
                    break;
            }
            GLsizei stride = size * sizeof(float);
            gl.glBufferData(GL_ARRAY_BUFFER, stride, (GLvoid*)&data[0], GL_STREAM_DRAW);
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

// API

void CoreProfileEngine::enable(GLenum cap) {
    switch (cap) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
        case GL_TEXTURE_GEN_STR_OES:
        case GL_RESCALE_NORMAL:
        case GL_NORMALIZE:
        case GL_LIGHTING:
        case GL_LIGHT0:
        case GL_LIGHT1:
        case GL_LIGHT2:
        case GL_LIGHT3:
        case GL_LIGHT4:
        case GL_LIGHT5:
        case GL_LIGHT6:
        case GL_LIGHT7:
        case GL_COLOR_MATERIAL:
        case GL_FOG:
        case GL_POINT_SMOOTH:
        case GL_ALPHA_TEST:
            return;
        default:
            break;
    }

    auto& gl = GLEScontext::dispatcher();
    gl.glEnable(cap);
}

void CoreProfileEngine::disable(GLenum cap) {
    switch (cap) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
        case GL_TEXTURE_GEN_STR_OES:
        case GL_RESCALE_NORMAL:
        case GL_NORMALIZE:
        case GL_LIGHTING:
        case GL_LIGHT0:
        case GL_LIGHT1:
        case GL_LIGHT2:
        case GL_LIGHT3:
        case GL_LIGHT4:
        case GL_LIGHT5:
        case GL_LIGHT6:
        case GL_LIGHT7:
        case GL_COLOR_MATERIAL:
        case GL_FOG:
        case GL_POINT_SMOOTH:
        case GL_ALPHA_TEST:
            return;
        default:
            break;
    }

    auto& gl = GLEScontext::dispatcher();
    gl.glDisable(cap);
}

void CoreProfileEngine::shadeModel(GLenum mode) {
    // no-op
}

void CoreProfileEngine::matrixMode(GLenum mode) {
    // no-op
}

void CoreProfileEngine::loadIdentity() {
    // no-op
}

void CoreProfileEngine::loadMatrixf(const GLfloat* m) {
    // no-op
}

void CoreProfileEngine::pushMatrix() {
    // no-op
}

void CoreProfileEngine::popMatrix() {
    // no-op
}

void CoreProfileEngine::multMatrixf(const GLfloat* m) {
    // no-op
}

void CoreProfileEngine::orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    // no-op
}

void CoreProfileEngine::frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    // no-op
}

void CoreProfileEngine::texEnvf(GLenum target, GLenum pname, GLfloat param) {
    // no-op
}

void CoreProfileEngine::texEnvfv(GLenum target, GLenum pname, const GLfloat* params) {
    // no-op
}

void CoreProfileEngine::texEnvi(GLenum target, GLenum pname, GLint param) {
    // no-op
}

void CoreProfileEngine::texEnviv(GLenum target, GLenum pname, const GLint* params) {
    // no-op
}

void CoreProfileEngine::getTexEnvfv(GLenum env, GLenum pname, GLfloat* params) {
    // no-op
}

void CoreProfileEngine::getTexEnviv(GLenum env, GLenum pname, GLint* params) {
    // no-op
}

void CoreProfileEngine::texGenf(GLenum coord, GLenum pname, GLfloat param) {
    // no-op
}

void CoreProfileEngine::texGenfv(GLenum coord, GLenum pname, const GLfloat* params) {
    // no-op
}

void CoreProfileEngine::texGeni(GLenum coord, GLenum pname, GLint param) {
    // no-op
}

void CoreProfileEngine::texGeniv(GLenum coord, GLenum pname, const GLint* params) {
    // no-op
}

void CoreProfileEngine::getTexGeniv(GLenum coord, GLenum pname, GLint* params) {
    // no-op
}

void CoreProfileEngine::getTexGenfv(GLenum coord, GLenum pname, GLfloat* params) {
    // no-op
}

void CoreProfileEngine::enableClientState(GLenum clientState) {
    // no-op
}

void CoreProfileEngine::disableClientState(GLenum clientState) {
    // no-op
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
    // This is not strictly needed, but Swiftshader indirect VAO
    // can forget its ELEMENT_ARRAY_BUFFER binding.
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_drawTexOESCoreState.ibo);

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
    // no-op
}

void CoreProfileEngine::scalef(float x, float y, float z) {
    // no-op
}

void CoreProfileEngine::translatef(float x, float y, float z) {
    // no-op
}

void CoreProfileEngine::color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    // no-op
}

void CoreProfileEngine::color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    // no-op
}

void CoreProfileEngine::activeTexture(GLenum unit) {
    // Use 2 texture image units for each texture unit of GLES1.
    // This allows us to simultaneously use GL_TEXTURE_2D
    // and GL_TEXTURE_CUBE_MAP with the same texture unit through
    // using different samplers for the image units.
    GLEScontext::dispatcher().glActiveTexture(
        GL_TEXTURE0 + mCtx->getActiveTextureUnit() * 2);
}

void CoreProfileEngine::clientActiveTexture(GLenum unit) {
    GLEScontext::dispatcher().glActiveTexture(
        GL_TEXTURE0 + mCtx->getActiveTextureUnit() * 2);
}


void CoreProfileEngine::preDrawTextureUnitEmulation() {
    auto& gl = GLEScontext::dispatcher();
    unsigned int currTextureUnit = mCtx->getActiveTextureUnit();

    gl.glUniform1i(m_geometryDrawState.enableTextureLoc,
                   mCtx->isEnabled(GL_TEXTURE_2D) &&
                   mCtx->isArrEnabled(GL_TEXTURE_COORD_ARRAY));

    gl.glUniform1i(m_geometryDrawState.textureSamplerLoc, currTextureUnit * 2);
    gl.glUniform1i(m_geometryDrawState.textureCubeSamplerLoc, currTextureUnit * 2 + 1);

    if (auto cubeMapTex = mCtx->getBindedTexture(currTextureUnit + GL_TEXTURE0, GL_TEXTURE_CUBE_MAP)) {
        GLuint cubeMapTexGlobal = mCtx->shareGroup()->getGlobalName(
                                      NamedObjectType::TEXTURE, cubeMapTex);
        gl.glActiveTexture(GL_TEXTURE0 + currTextureUnit * 2);
        gl.glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        gl.glActiveTexture(GL_TEXTURE0 + currTextureUnit * 2 + 1);
        gl.glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexGlobal);
        gl.glActiveTexture(GL_TEXTURE0 + currTextureUnit * 2);
    }

    GLenum textureGenMode = mCtx->getTextureGenMode();
    if (textureGenMode == GL_REFLECTION_MAP_OES) {
        gl.glUniform1i(m_geometryDrawState.enableTextureLoc, 1);
        gl.glUniform1i(m_geometryDrawState.enableReflectionMapLoc, 1);
    } else {
        gl.glUniform1i(m_geometryDrawState.enableReflectionMapLoc, 0);
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

    gl.glUniform1i(m_geometryDrawState.enableLightingLoc, 0);
    gl.glUniform1i(m_geometryDrawState.textureEnvModeLoc, mCtx->getTextureEnvMode());
}

void CoreProfileEngine::postDrawTextureUnitEmulation() {
    auto& gl = GLEScontext::dispatcher();
    unsigned int currTextureUnit = mCtx->getActiveTextureUnit();

    GLuint cubeMapTex = mCtx->getBindedTexture(currTextureUnit + GL_TEXTURE0, GL_TEXTURE_CUBE_MAP);

    if (cubeMapTex) {
        GLuint cubeMapTexGlobal = mCtx->shareGroup()->getGlobalName(
                                      NamedObjectType::TEXTURE, cubeMapTex);
        gl.glActiveTexture(GL_TEXTURE0 + currTextureUnit * 2);
        gl.glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexGlobal);
        gl.glActiveTexture(GL_TEXTURE0 + currTextureUnit * 2 + 1);
        gl.glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        gl.glActiveTexture(GL_TEXTURE0 + currTextureUnit * 2);
    }
}

void CoreProfileEngine::preDrawVertexSetup() {
    auto& gl = GLEScontext::dispatcher();

    glm::mat4 currProjMatrix = mCtx->getProjMatrix();
    glm::mat4 currModelviewMatrix = mCtx->getModelviewMatrix();
    glm::mat4 currTextureMatrix = mCtx->getTextureMatrix();
    glm::mat4 currModelviewMatrixInvTr = glm::inverseTranspose(currModelviewMatrix);

    gl.glBindVertexArray(m_geometryDrawState.vao);

    gl.glUseProgram(mCtx->getShadeModel() == GL_FLAT ?
                    m_geometryDrawState.programFlat :
                    m_geometryDrawState.program);
    gl.glUniformMatrix4fv(m_geometryDrawState.projMatrixLoc, 1, GL_FALSE, glm::value_ptr(currProjMatrix));
    gl.glUniformMatrix4fv(m_geometryDrawState.modelviewMatrixLoc, 1, GL_FALSE, glm::value_ptr(currModelviewMatrix));
    gl.glUniformMatrix4fv(m_geometryDrawState.modelviewInvTrLoc, 1, GL_FALSE, glm::value_ptr(currModelviewMatrixInvTr));
    gl.glUniformMatrix4fv(m_geometryDrawState.textureMatrixLoc, 1, GL_FALSE, glm::value_ptr(currTextureMatrix));
}

void CoreProfileEngine::postDrawVertexSetup() {
    auto& gl = GLEScontext::dispatcher();

    gl.glBindVertexArray(0);
}

void CoreProfileEngine::setupLighting() {
    auto& gl = GLEScontext::dispatcher();

    gl.glUniform1i(m_geometryDrawState.enableLightingLoc,
                   mCtx->isEnabled(GL_LIGHTING));
    gl.glUniform1i(m_geometryDrawState.enableRescaleNormalLoc,
                   mCtx->isEnabled(GL_RESCALE_NORMAL));
    gl.glUniform1i(m_geometryDrawState.enableNormalizeLoc,
                   mCtx->isEnabled(GL_NORMALIZE));
    gl.glUniform1i(m_geometryDrawState.enableColorMaterialLoc,
                   mCtx->isEnabled(GL_COLOR_MATERIAL));

    const auto& material = mCtx->getMaterialInfo();

    gl.glUniform4fv(m_geometryDrawState.materialAmbientLoc, 1, material.ambient);
    gl.glUniform4fv(m_geometryDrawState.materialDiffuseLoc, 1, material.diffuse);
    gl.glUniform4fv(m_geometryDrawState.materialSpecularLoc, 1, material.specular);
    gl.glUniform4fv(m_geometryDrawState.materialEmissiveLoc, 1, material.emissive);
    gl.glUniform1f(m_geometryDrawState.materialSpecularExponentLoc, material.specularExponent);

    const auto& lightModel = mCtx->getLightModelInfo();

    gl.glUniform4fv(m_geometryDrawState.lightModelSceneAmbientLoc, 1, lightModel.color);
    gl.glUniform1i(m_geometryDrawState.lightModelTwoSidedLoc, lightModel.twoSided);

    assert(kMaxLights == GLEScmContext::kMaxLights);

    for (int i = 0; i < GLEScmContext::kMaxLights; i++) {
        m_lightingBuffer.lightEnables[i] = mCtx->isEnabled(GL_LIGHT0 + i);
        const auto& light = mCtx->getLightInfo(i);
        memcpy(m_lightingBuffer.lightAmbients + 4 * i, &light.ambient, 4 * sizeof(GLfloat));
        memcpy(m_lightingBuffer.lightDiffuses + 4 * i, &light.diffuse, 4 * sizeof(GLfloat));
        memcpy(m_lightingBuffer.lightSpeculars + 4 * i, &light.specular, 4 * sizeof(GLfloat));
        memcpy(m_lightingBuffer.lightPositions + 4 * i, &light.position, 4 * sizeof(GLfloat));
        memcpy(m_lightingBuffer.lightDirections + 3 * i, &light.direction, 3 * sizeof(GLfloat));
        m_lightingBuffer.spotlightExponents[i] = light.spotlightExponent;
        m_lightingBuffer.spotlightCutoffAngles[i] = light.spotlightCutoffAngle;
        m_lightingBuffer.attenuationConsts[i] = light.attenuationConst;
        m_lightingBuffer.attenuationLinears[i] = light.attenuationLinear;
        m_lightingBuffer.attenuationQuadratics[i] = light.attenuationQuadratic;
    }

    gl.glUniform1iv(m_geometryDrawState.lightEnablesLoc, GLEScmContext::kMaxLights, m_lightingBuffer.lightEnables);
    gl.glUniform4fv(m_geometryDrawState.lightAmbientsLoc, GLEScmContext::kMaxLights, m_lightingBuffer.lightAmbients);
    gl.glUniform4fv(m_geometryDrawState.lightDiffusesLoc, GLEScmContext::kMaxLights, m_lightingBuffer.lightDiffuses);
    gl.glUniform4fv(m_geometryDrawState.lightSpecularsLoc, GLEScmContext::kMaxLights, m_lightingBuffer.lightSpeculars);
    gl.glUniform4fv(m_geometryDrawState.lightPositionsLoc, GLEScmContext::kMaxLights, m_lightingBuffer.lightPositions);
    gl.glUniform3fv(m_geometryDrawState.lightDirectionsLoc, GLEScmContext::kMaxLights, m_lightingBuffer.lightDirections);
    gl.glUniform1fv(m_geometryDrawState.lightSpotlightExponentsLoc, GLEScmContext::kMaxLights, m_lightingBuffer.spotlightExponents);
    gl.glUniform1fv(m_geometryDrawState.lightSpotlightCutoffAnglesLoc, GLEScmContext::kMaxLights, m_lightingBuffer.spotlightCutoffAngles);
    gl.glUniform1fv(m_geometryDrawState.lightAttenuationConstsLoc, GLEScmContext::kMaxLights, m_lightingBuffer.attenuationConsts);
    gl.glUniform1fv(m_geometryDrawState.lightAttenuationLinearsLoc, GLEScmContext::kMaxLights, m_lightingBuffer.attenuationLinears);
    gl.glUniform1fv(m_geometryDrawState.lightAttenuationQuadraticsLoc, GLEScmContext::kMaxLights, m_lightingBuffer.attenuationQuadratics);
}

void CoreProfileEngine::setupFog() {
    auto& gl = GLEScontext::dispatcher();

    gl.glUniform1i(m_geometryDrawState.enableFogLoc,
                   mCtx->isEnabled(GL_FOG));

    const auto& fogInfo = mCtx->getFogInfo();

    gl.glUniform1i(m_geometryDrawState.fogModeLoc, fogInfo.mode);
    gl.glUniform1f(m_geometryDrawState.fogDensityLoc, fogInfo.density);
    gl.glUniform1f(m_geometryDrawState.fogStartLoc, fogInfo.start);
    gl.glUniform1f(m_geometryDrawState.fogEndLoc, fogInfo.end);

    gl.glUniform4fv(m_geometryDrawState.fogColorLoc, 1, fogInfo.color);
}

void CoreProfileEngine::drawArrays(GLenum type, GLint first, GLsizei count) {
    auto& gl = GLEScontext::dispatcher();

    preDrawVertexSetup();
    preDrawTextureUnitEmulation();

    setupLighting();
    setupFog();

    gl.glDrawArrays(type, first, count);

    postDrawVertexSetup();
    postDrawTextureUnitEmulation();
}

void CoreProfileEngine::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    auto& gl = GLEScontext::dispatcher();

    preDrawVertexSetup();
    preDrawTextureUnitEmulation();

    setupLighting();
    setupFog();

    gl.glDrawElements(mode, count, type, (GLvoid*)0);

    postDrawVertexSetup();
    postDrawTextureUnitEmulation();
}
