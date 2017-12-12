/*
* Copyright (C) 2011 The Android Open Source Project
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
#ifndef GLES_CM_CONTEX_H
#define GLES_CM_CONTEX_H

#include "CoreProfileEngine.h"

#include <GLcommon/GLDispatch.h>
#include <GLcommon/GLESpointer.h>
#include <GLcommon/GLESbuffer.h>
#include <GLcommon/GLEScontext.h>

#include <glm/mat4x4.hpp>

#include <vector>
#include <string>
#include <unordered_map>

typedef std::unordered_map<GLfloat,std::vector<int> > PointSizeIndices;

class GlLibrary;
class CoreProfileEngine;

class GLEScmContext: public GLEScontext
{
public:
    virtual void init();
    static void initGlobal(EGLiface* eglIface);
    GLEScmContext(int maj, int min, GlobalNameSpace* globalNameSpace,
            android::base::Stream* stream);
    void setActiveTexture(GLenum tex);
    void  setClientActiveTexture(GLenum tex);
    GLenum  getActiveTexture() { return GL_TEXTURE0 + m_activeTexture;};
    GLenum  getClientActiveTexture() { return GL_TEXTURE0 + m_clientActiveTexture;};
    void setBindedTexture(GLenum target, unsigned int texture, unsigned int globalTexName);
    void setupArraysPointers(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct);
    void drawPointsArrs(GLESConversionArrays& arrs,GLint first,GLsizei count);
    void drawPointsElems(GLESConversionArrays& arrs,GLsizei count,GLenum type,const GLvoid* indices);
    virtual const GLESpointer* getPointer(GLenum arrType);
    int  getMaxTexUnits();

    virtual void initDefaultFBO(
            GLint width, GLint height, GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
            GLuint* eglSurfaceRBColorId, GLuint* eglSurfaceRBDepthId,
            GLuint readWidth, GLint readHeight, GLint readColorFormat, GLint readDepthStencilFormat, GLint readMultisamples,
            GLuint* eglReadSurfaceRBColorId, GLuint* eglReadSurfaceRBDepthId) override;
    ~GLEScmContext();
    static void setMaxGlesVersion(GLESVersion version);

    // Emulated GLES1

    // Errors coming from emulation on core profile
    GLint getErrorCoreProfile();

    // API
    virtual bool glGetIntegerv(GLenum pname, GLint *params);
    virtual bool glGetBooleanv(GLenum pname, GLboolean *params);
    virtual bool glGetFloatv(GLenum pname, GLfloat *params);
    virtual bool glGetFixedv(GLenum pname, GLfixed *params);

    void enable(GLenum cap);
    void disable(GLenum cap);

    void shadeModel(GLenum mode);
    GLenum getShadeModel() const { return mShadeModel; }

    void matrixMode(GLenum mode);
    void loadIdentity();
    void loadMatrixf(const GLfloat* m);
    void pushMatrix();
    void popMatrix();
    void multMatrixf(const GLfloat* m);

    void orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
    void frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);

    void texEnvf(GLenum target, GLenum pname, GLfloat param);
    void texEnvfv(GLenum target, GLenum pname, const GLfloat* params);
    void texEnvi(GLenum target, GLenum pname, GLint param);
    void texEnviv(GLenum target, GLenum pname, const GLint* params);
    void getTexEnvfv(GLenum env, GLenum pname, GLfloat* params);
    void getTexEnviv(GLenum env, GLenum pname, GLint* params);

    void texGenf(GLenum coord, GLenum pname, GLfloat param);
    void texGenfv(GLenum coord, GLenum pname, const GLfloat* params);
    void texGeni(GLenum coord, GLenum pname, GLint param);
    void texGeniv(GLenum coord, GLenum pname, const GLint* params);
    void getTexGeniv(GLenum coord, GLenum pname, GLint* params);
    void getTexGenfv(GLenum coord, GLenum pname, GLfloat* params);

    void materialf(GLenum face, GLenum pname, GLfloat param);
    void materialfv(GLenum face, GLenum pname, const GLfloat* params);
    void getMaterialfv(GLenum face, GLenum pname, GLfloat* params);

    void lightModelf(GLenum pname, GLfloat param);
    void lightModelfv(GLenum pname, const GLfloat* params);
    void lightf(GLenum light, GLenum pname, GLfloat param);
    void lightfv(GLenum light, GLenum pname, const GLfloat* params);
    void getLightfv(GLenum light, GLenum pname, GLfloat* params);

    void multiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
    void normal3f(GLfloat nx, GLfloat ny, GLfloat nz);

    void fogf(GLenum pname, GLfloat param);
    void fogfv(GLenum pname, const GLfloat* params);

    void enableClientState(GLenum clientState);
    void disableClientState(GLenum clientState);

    void drawTexOES(float x, float y, float z, float width, float height);

    void rotatef(float angle, float x, float y, float z);
    void scalef(float x, float y, float z);
    void translatef(float x, float y, float z);

    void color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

    void drawArrays(GLenum mode, GLint first, GLsizei count);
    void drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);

    void clientActiveTexture(GLenum texture);

    bool doConvert(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id);

    std::vector<float> getColor() const;
    std::vector<float> getNormal() const;
    std::vector<float> getMultiTexCoord(uint32_t index) const;
    GLenum getTextureEnvMode();
    GLenum getTextureGenMode();

    glm::mat4 getProjMatrix();
    glm::mat4 getModelviewMatrix();
    glm::mat4 getTextureMatrix();

    struct Material {
        GLfloat ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
        GLfloat diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
        GLfloat specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat emissive[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat specularExponent = 0.0f;
    };

    struct LightModel {
        GLfloat color[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
        bool twoSided = false;
    };

    struct Light {
        GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat position[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
        GLfloat direction[3] = { 0.0f, 0.0f, -1.0f };
        GLfloat spotlightExponent = 0.0f;
        GLfloat spotlightCutoffAngle = 180.0f;
        GLfloat attenuationConst = 1.0f;
        GLfloat attenuationLinear = 0.0f;
        GLfloat attenuationQuadratic = 0.0f;
    };

    struct Fog {
        GLenum mode = GL_EXP;
        float density = 1.0f;
        float start = 0.0f;
        float end = 1.0f;
        GLfloat color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    static constexpr int kMaxLights = 8;
    const Material& getMaterialInfo();
    const LightModel& getLightModelInfo();
    const Light& getLightInfo(uint32_t lightIndex);
    const Fog& getFogInfo();

    virtual void onSave(android::base::Stream* stream) const override;

protected:
    virtual void postLoadRestoreCtx() override;

    static const GLint kMaxTextureUnits = 4;
    static const GLint kMaxMatrixStackSize = 16;

    bool needConvert(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id);
private:
    void setupArrayPointerHelper(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLenum array_id,GLESpointer* p);
    void setupArr(const GLvoid* arr,GLenum arrayType,GLenum dataType,GLint size,GLsizei stride,GLboolean normalized, int pointsIndex = -1, bool isInt = false);
    void drawPoints(PointSizeIndices* points);
    void drawPointsData(GLESConversionArrays& arrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices_in,bool isElemsDraw);
    void initExtensionString();
    void restoreVertexAttrib(GLenum attrib);
    CoreProfileEngine& core() { return *m_coreProfileEngine; }

    GLESpointer*          m_texCoords = nullptr;
    int                   m_pointsIndex = -1;
    unsigned int          m_clientActiveTexture = 0;

    GLenum mShadeModel = GL_SMOOTH;

    union GLVal {
        GLfloat floatVal[4];
        GLint intVal[4];
        GLubyte ubyteVal[4];
        GLenum enumVal[4];
    };

    struct GLValTyped {
        GLenum type;
        GLVal val;
    };

    using TexEnv = std::unordered_map<GLenum, GLValTyped>;
    using TexUnitEnvs = std::vector<TexEnv>;
    using TexGens = std::vector<TexEnv>;
    using MatrixStack = std::vector<glm::mat4>;

    GLenum mCurrMatrixMode = GL_PROJECTION;

    GLValTyped mColor;
    GLValTyped mNormal;
    GLVal mMultiTexCoord[kMaxTextureUnits] = {};

    TexUnitEnvs mTexUnitEnvs;
    TexGens mTexGens;

    MatrixStack mProjMatrices;
    MatrixStack mModelviewMatrices;
    std::vector<MatrixStack> mTextureMatrices;
    glm::mat4& currMatrix();
    MatrixStack& currMatrixStack();
    void restoreMatrixStack(const MatrixStack& matrices);

    Material mMaterial;
    LightModel mLightModel;
    Light mLights[kMaxLights] = {};
    Fog mFog = {};

    // Core profile stuff
    CoreProfileEngine*    m_coreProfileEngine = nullptr;
};

#endif
