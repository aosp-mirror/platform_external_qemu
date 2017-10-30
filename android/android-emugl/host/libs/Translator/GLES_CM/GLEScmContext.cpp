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

#include "GLEScmContext.h"
#include "GLEScmUtils.h"
#include <GLcommon/GLutils.h>
#include <GLcommon/GLconversion_macros.h>
#include <string.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include "emugl/common/crash_reporter.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static GLESVersion s_maxGlesVersion = GLES_1_1;

void GLEScmContext::setMaxGlesVersion(GLESVersion version) {
    s_maxGlesVersion = version;
}

void GLEScmContext::init(EGLiface* eglIface) {
    emugl::Mutex::AutoLock mutex(s_lock);
    if(!m_initialized || m_needRestoreFromSnapshot) {
        s_glDispatch.dispatchFuncs(s_maxGlesVersion, eglIface->eglGetGlLibrary());
        GLEScontext::init(eglIface);

        m_texCoords = new GLESpointer[s_glSupport.maxTexUnits];
        m_currVaoState[GL_TEXTURE_COORD_ARRAY]  = &m_texCoords[m_clientActiveTexture];

        buildStrings((const char*)dispatcher().glGetString(GL_VENDOR),
                     (const char*)dispatcher().glGetString(GL_RENDERER),
                     (const char*)dispatcher().glGetString(GL_VERSION),
                     "OpenGL ES-CM 1.1");

        if (isCoreProfile()) {
            m_coreProfileEngine = new CoreProfileEngine(this);
        }
    }
    m_initialized = true;
    m_needRestoreFromSnapshot = false;
}

void GLEScmContext::initDefaultFBO(
        GLint width, GLint height, GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
        GLuint* eglSurfaceRBColorId, GLuint* eglSurfaceRBDepthId,
        GLuint readWidth, GLint readHeight, GLint readColorFormat, GLint readDepthstencilFormat, GLint readMultisamples,
        GLuint* eglReadSurfaceRBColorId, GLuint* eglReadSurfaceRBDepthId) {
    GLEScontext::initDefaultFBO(
            width, height, colorFormat, depthstencilFormat, multisamples,
            eglSurfaceRBColorId, eglSurfaceRBDepthId,
            readWidth, readHeight, readColorFormat, readDepthstencilFormat, readMultisamples,
            eglReadSurfaceRBColorId, eglReadSurfaceRBDepthId
            );
}

GLEScmContext::GLEScmContext(int maj, int min,
        GlobalNameSpace* globalNameSpace, android::base::Stream* stream)
    : GLEScontext(globalNameSpace, stream, nullptr) {
    // TODO: snapshot support
    m_glesMajorVersion = maj;
    m_glesMinorVersion = min;
    addVertexArrayObject(0);
    setVertexArrayObject(0);

    m_currVaoState[GL_COLOR_ARRAY]          = new GLESpointer();
    m_currVaoState[GL_NORMAL_ARRAY]         = new GLESpointer();
    m_currVaoState[GL_VERTEX_ARRAY]         = new GLESpointer();
    m_currVaoState[GL_POINT_SIZE_ARRAY_OES] = new GLESpointer();

    mProjMatrices.resize(1, glm::mat4());
    mModelviewMatrices.resize(1, glm::mat4());
    mTextureMatrices.resize(kMaxTextureUnits, { glm::mat4() });
    mTexUnitEnvs.resize(kMaxTextureUnits, {});
    mTexGens.resize(kMaxTextureUnits, {});

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


void GLEScmContext::setActiveTexture(GLenum tex) {
   m_activeTexture = tex - GL_TEXTURE0;
}

void GLEScmContext::setClientActiveTexture(GLenum tex) {
   m_clientActiveTexture = tex - GL_TEXTURE0;
   m_currVaoState[GL_TEXTURE_COORD_ARRAY] = &m_texCoords[m_clientActiveTexture];
}

void GLEScmContext::setBindedTexture(GLenum target, unsigned int texture, unsigned int globalTexName) {
    GLEScontext::setBindedTexture(target, texture);
}

GLEScmContext::~GLEScmContext(){
    if(m_texCoords){
        delete[] m_texCoords;
        m_texCoords = NULL;
    }
    m_currVaoState[GL_TEXTURE_COORD_ARRAY] = NULL;

    if (m_coreProfileEngine) {
        delete m_coreProfileEngine;
        m_coreProfileEngine = NULL;
    }
}


//setting client side arr
void GLEScmContext::setupArr(const GLvoid* arr,GLenum arrayType,GLenum dataType,GLint size,GLsizei stride,GLboolean normalized, int index, bool isInt){
    if( arr == NULL) return;
    switch(arrayType) {
        case GL_VERTEX_ARRAY:
            s_glDispatch.glVertexPointer(size,dataType,stride,arr);
            break;
        case GL_NORMAL_ARRAY:
            s_glDispatch.glNormalPointer(dataType,stride,arr);
            break;
        case GL_TEXTURE_COORD_ARRAY:
            s_glDispatch.glTexCoordPointer(size,dataType,stride,arr);
            break;
        case GL_COLOR_ARRAY:
            s_glDispatch.glColorPointer(size,dataType,stride,arr);
            break;
        case GL_POINT_SIZE_ARRAY_OES:
            m_pointsIndex = index;
            break;
    }
}


void GLEScmContext::setupArrayPointerHelper(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLenum array_id,GLESpointer* p){
        unsigned int size = p->getSize();
        GLenum dataType = p->getType();

        if(needConvert(cArrs,first,count,type,indices,direct,p,array_id)){
            //conversion has occured
            ArrayData currentArr = cArrs.getCurrentArray();
            setupArr(currentArr.data,array_id,currentArr.type,size,currentArr.stride,GL_FALSE, cArrs.getCurrentIndex());
            ++cArrs;
        } else {
            setupArr(p->getData(),array_id,dataType,size,p->getStride(), GL_FALSE);
        }
}

void GLEScmContext::setupArraysPointers(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct) {
    ArraysMap::iterator it;
    m_pointsIndex = -1;

    //going over all clients arrays Pointers
    for ( it=m_currVaoState.begin() ; it != m_currVaoState.end(); ++it) {

        GLenum array_id   = (*it).first;
        GLESpointer* p = (*it).second;
        if(!p->isEnable()) continue;
        if(array_id == GL_TEXTURE_COORD_ARRAY) continue; //handling textures later
        setupArrayPointerHelper(cArrs,first,count,type,indices,direct,array_id,p);
    }

    unsigned int activeTexture = m_clientActiveTexture + GL_TEXTURE0;

    s_lock.lock();
    int maxTexUnits = s_glSupport.maxTexUnits;
    s_lock.unlock();

    //converting all texture coords arrays
    for(int i=0; i< maxTexUnits;i++) {

        unsigned int tex = GL_TEXTURE0+i;
        setClientActiveTexture(tex);
        s_glDispatch.glClientActiveTexture(tex);

        GLenum array_id   = GL_TEXTURE_COORD_ARRAY;
        GLESpointer* p = m_currVaoState[array_id];
        if(!p->isEnable()) continue;
        setupArrayPointerHelper(cArrs,first,count,type,indices,direct,array_id,p);
    }

    setClientActiveTexture(activeTexture);
    s_glDispatch.glClientActiveTexture(activeTexture);
}

void  GLEScmContext::drawPointsData(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices_in,bool isElemsDraw) {
    const char  *pointsArr =  NULL;
    int stride = 0;
    GLESpointer* p = m_currVaoState[GL_POINT_SIZE_ARRAY_OES];

    //choosing the right points sizes array source
    if(m_pointsIndex >= 0) { //point size array was converted
        pointsArr = (const char*)(cArrs[m_pointsIndex].data);
        stride = cArrs[m_pointsIndex].stride;
    } else {
        pointsArr = static_cast<const char*>(p->getData());
        stride = p->getStride();
    }

    if(stride == 0){
        stride = sizeof(GLfloat);
    }


    if(isElemsDraw) {
        int tSize = 0;
        switch (type) {
            case GL_UNSIGNED_BYTE:
                tSize = 1;
                break;
            case GL_UNSIGNED_SHORT:
                tSize = 2;
                break;
            case GL_UNSIGNED_INT:
                tSize = 4;
                break;
        };

        int i = 0;
        while(i<count)
        {
            int sStart = i;
            int sCount = 1;

#define INDEX \
                (type == GL_UNSIGNED_INT ? \
                static_cast<const GLuint*>(indices_in)[i]: \
                type == GL_UNSIGNED_SHORT ? \
                static_cast<const GLushort*>(indices_in)[i]: \
                static_cast<const GLubyte*>(indices_in)[i])

            GLfloat pSize = *((GLfloat*)(pointsArr+(INDEX*stride)));
            i++;

            while(i < count && pSize == *((GLfloat*)(pointsArr+(INDEX*stride))))
            {
                sCount++;
                i++;
            }

            s_glDispatch.glPointSize(pSize);
            s_glDispatch.glDrawElements(GL_POINTS, sCount, type, (char*)indices_in+sStart*tSize);
        }
    } else {
        int i = 0;
        while(i<count)
        {
            int sStart = i;
            int sCount = 1;
            GLfloat pSize = *((GLfloat*)(pointsArr+((first+i)*stride)));
            i++;

            while(i < count && pSize == *((GLfloat*)(pointsArr+((first+i)*stride))))
            {
                sCount++;
                i++;
            }

            s_glDispatch.glPointSize(pSize);
            s_glDispatch.glDrawArrays(GL_POINTS, first+sStart, sCount);
        }
    }
}

void  GLEScmContext::drawPointsArrs(GLESConversionArrays& arrs,GLint first,GLsizei count) {
    drawPointsData(arrs,first,count,0,NULL,false);
}

void GLEScmContext::drawPointsElems(GLESConversionArrays& arrs,GLsizei count,GLenum type,const GLvoid* indices_in) {
    drawPointsData(arrs,0,count,type,indices_in,true);
}

bool GLEScmContext::doConvert(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id) {
    return needConvert(cArrs, first, count, type, indices, direct, p, array_id);
}

std::vector<float> GLEScmContext::getColor() const {
    switch (mColor.type) {
        case GL_UNSIGNED_BYTE:
            return
            { mColor.val.ubyteVal[0] / 255.0f,
              mColor.val.ubyteVal[1] / 255.0f,
              mColor.val.ubyteVal[2] / 255.0f,
              mColor.val.ubyteVal[3] / 255.0f, };
        default:
        case GL_FLOAT:
            return { mColor.val.floatVal[0],
                     mColor.val.floatVal[1],
                     mColor.val.floatVal[2],
                     mColor.val.floatVal[3], };
    }
}

std::vector<float> GLEScmContext::getNormal() const {
    return
        { mNormal.val.floatVal[0],
          mNormal.val.floatVal[1],
          mNormal.val.floatVal[2] };
}

std::vector<float> GLEScmContext::getMultiTexCoord(uint32_t index) const {
    return // s, t, r components
        { mMultiTexCoord[index].floatVal[0],
          mMultiTexCoord[index].floatVal[1],
          mMultiTexCoord[index].floatVal[2], };
}

GLenum GLEScmContext::getTextureEnvMode() {
    return mTexUnitEnvs[m_activeTexture][GL_TEXTURE_ENV_MODE].intVal[0];
}

GLenum GLEScmContext::getTextureGenMode() {
    return mTexGens[m_activeTexture][GL_TEXTURE_GEN_MODE_OES].intVal[0];
}

glm::mat4 GLEScmContext::getProjMatrix() {
    return mProjMatrices.back();
}

glm::mat4 GLEScmContext::getModelviewMatrix() {
    return mModelviewMatrices.back();
}

glm::mat4 GLEScmContext::getTextureMatrix() {
    return mTextureMatrices[m_activeTexture].back();
}

glm::mat4& GLEScmContext::currMatrix() {
    return currMatrixStack().back();
}

GLEScmContext::MatrixStack& GLEScmContext::currMatrixStack() {
    switch (mCurrMatrixMode) {
    case GL_TEXTURE:
        return mTextureMatrices[m_activeTexture];
    case GL_PROJECTION:
        return mProjMatrices;
    case GL_MODELVIEW:
        return mModelviewMatrices;
    default:
        emugl_crash_reporter(
            "error: matrix mode set to 0x%x!",
            mCurrMatrixMode);
    }
    // Make compiler happy
    return mModelviewMatrices;
}

bool GLEScmContext::needConvert(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id) {

    bool usingVBO = p->getAttribType() == GLESpointer::BUFFER;
    GLenum arrType = p->getType();
    /*
     conversion is not necessary in the following cases:
      (*) array type is byte but it is not vertex or texture array
      (*) array type is not fixed
    */
    if((arrType != GL_FIXED) && (arrType != GL_BYTE)) return false;
    if((arrType == GL_BYTE   && (array_id != GL_VERTEX_ARRAY)) &&
       (arrType == GL_BYTE   && (array_id != GL_TEXTURE_COORD_ARRAY)) ) return false;


    bool byteVBO = (arrType == GL_BYTE) && usingVBO;
    if(byteVBO){
        p->redirectPointerData();
    }

    if(!usingVBO || byteVBO) {
        if (direct) {
            convertDirect(cArrs,first,count,array_id,p);
        } else {
            convertIndirect(cArrs,count,type,indices,array_id,p);
        }
    } else {
        if (direct) {
            convertDirectVBO(cArrs,first,count,array_id,p) ;
        } else {
            convertIndirectVBO(cArrs,count,type,indices,array_id,p);
        }
    }
    return true;
}

const GLESpointer* GLEScmContext::getPointer(GLenum arrType) {
    GLenum type =
        arrType == GL_VERTEX_ARRAY_POINTER          ? GL_VERTEX_ARRAY :
        arrType == GL_NORMAL_ARRAY_POINTER          ? GL_NORMAL_ARRAY :
        arrType == GL_TEXTURE_COORD_ARRAY_POINTER   ? GL_TEXTURE_COORD_ARRAY :
        arrType == GL_COLOR_ARRAY_POINTER           ? GL_COLOR_ARRAY :
        arrType == GL_POINT_SIZE_ARRAY_POINTER_OES  ? GL_POINT_SIZE_ARRAY_OES :
        0;
    if(type != 0)
    {
        return GLEScontext::getPointer(type);
    }
    return NULL;
}

void GLEScmContext::initExtensionString() {
    *s_glExtensions = "GL_OES_blend_func_separate GL_OES_blend_equation_separate GL_OES_blend_subtract "
                      "GL_OES_byte_coordinates GL_OES_compressed_paletted_texture GL_OES_point_size_array "
                      "GL_OES_point_sprite GL_OES_single_precision GL_OES_stencil_wrap GL_OES_texture_env_crossbar "
                      "GL_OES_texture_mirored_repeat GL_OES_EGL_image GL_OES_element_index_uint GL_OES_draw_texture "
                      "GL_OES_texture_cube_map GL_OES_draw_texture ";
    if (s_glSupport.GL_OES_READ_FORMAT)
        *s_glExtensions+="GL_OES_read_format ";
    if (s_glSupport.GL_EXT_FRAMEBUFFER_OBJECT) {
        *s_glExtensions+="GL_OES_framebuffer_object GL_OES_depth24 GL_OES_depth32 GL_OES_fbo_render_mipmap "
                         "GL_OES_rgb8_rgba8 GL_OES_stencil1 GL_OES_stencil4 GL_OES_stencil8 ";
    }
    if (s_glSupport.GL_EXT_PACKED_DEPTH_STENCIL)
        *s_glExtensions+="GL_OES_packed_depth_stencil ";
    if (s_glSupport.GL_EXT_TEXTURE_FORMAT_BGRA8888)
        *s_glExtensions+="GL_EXT_texture_format_BGRA8888 GL_APPLE_texture_format_BGRA8888 ";
    if (s_glSupport.GL_ARB_MATRIX_PALETTE && s_glSupport.GL_ARB_VERTEX_BLEND) {
        *s_glExtensions+="GL_OES_matrix_palette ";
        GLint max_palette_matrices=0;
        GLint max_vertex_units=0;
        dispatcher().glGetIntegerv(GL_MAX_PALETTE_MATRICES_OES,&max_palette_matrices);
        dispatcher().glGetIntegerv(GL_MAX_VERTEX_UNITS_OES,&max_vertex_units);
        if (max_palette_matrices>=32 && max_vertex_units>=4)
            *s_glExtensions+="GL_OES_extended_matrix_palette ";
    }
    *s_glExtensions+="GL_OES_compressed_ETC1_RGB8_texture ";
}

int GLEScmContext::getMaxTexUnits() {
    return getCaps()->maxTexUnits;
}

bool GLEScmContext::glGetBooleanv(GLenum pname, GLboolean *params)
{
    GLint iParam;

    if(glGetIntegerv(pname, &iParam))
    {
        *params = (iParam != 0);
        return true;
    }

    return false;
}

bool GLEScmContext::glGetFixedv(GLenum pname, GLfixed *params)
{
    GLint iParam;

    if(glGetIntegerv(pname, &iParam))
    {
        *params = I2X(iParam);
        return true;
    }

    return false;
}

bool GLEScmContext::glGetFloatv(GLenum pname, GLfloat *params)
{
    GLint iParam;

    if(glGetIntegerv(pname, &iParam))
    {
        *params = (GLfloat)iParam;
        return true;
    }

    return false;
}

bool GLEScmContext::glGetIntegerv(GLenum pname, GLint *params)
{
    if(GLEScontext::glGetIntegerv(pname, params))
        return true;

    const GLESpointer* ptr = NULL;

    switch(pname){
        case GL_VERTEX_ARRAY_BUFFER_BINDING:
        case GL_VERTEX_ARRAY_SIZE:
        case GL_VERTEX_ARRAY_STRIDE:
        case GL_VERTEX_ARRAY_TYPE:
            ptr = getPointer(GL_VERTEX_ARRAY_POINTER);
            break;

        case GL_NORMAL_ARRAY_BUFFER_BINDING:
        case GL_NORMAL_ARRAY_STRIDE:
        case GL_NORMAL_ARRAY_TYPE:
            ptr = getPointer(GL_NORMAL_ARRAY_POINTER);
            break;

        case GL_COLOR_ARRAY_BUFFER_BINDING:
        case GL_COLOR_ARRAY_SIZE:
        case GL_COLOR_ARRAY_STRIDE:
        case GL_COLOR_ARRAY_TYPE:
            ptr = getPointer(GL_COLOR_ARRAY_POINTER);
            break;

        case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
        case GL_TEXTURE_COORD_ARRAY_SIZE:
        case GL_TEXTURE_COORD_ARRAY_STRIDE:
        case GL_TEXTURE_COORD_ARRAY_TYPE:
            ptr = getPointer(GL_TEXTURE_COORD_ARRAY_POINTER);
            break;

        case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
        case GL_POINT_SIZE_ARRAY_STRIDE_OES:
        case GL_POINT_SIZE_ARRAY_TYPE_OES:
            ptr = getPointer(GL_POINT_SIZE_ARRAY_POINTER_OES);
            break;

        default:
            return false;
    }

    switch(pname)
    {
        case GL_VERTEX_ARRAY_BUFFER_BINDING:
        case GL_NORMAL_ARRAY_BUFFER_BINDING:
        case GL_COLOR_ARRAY_BUFFER_BINDING:
        case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
        case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
            *params = ptr ? ptr->getBufferName() : 0;
            break;

        case GL_VERTEX_ARRAY_STRIDE:
        case GL_NORMAL_ARRAY_STRIDE:
        case GL_COLOR_ARRAY_STRIDE:
        case GL_TEXTURE_COORD_ARRAY_STRIDE:
        case GL_POINT_SIZE_ARRAY_STRIDE_OES:
            *params = ptr ? ptr->getStride() : 0;
            break;

        case GL_VERTEX_ARRAY_SIZE:
        case GL_COLOR_ARRAY_SIZE:
        case GL_TEXTURE_COORD_ARRAY_SIZE:
            *params = ptr ? ptr->getSize() : 0;
            break;

        case GL_VERTEX_ARRAY_TYPE:
        case GL_NORMAL_ARRAY_TYPE:
        case GL_COLOR_ARRAY_TYPE:
        case GL_TEXTURE_COORD_ARRAY_TYPE:
        case GL_POINT_SIZE_ARRAY_TYPE_OES:
            *params = ptr ? ptr->getType() : 0;
            break;
    }

    return true;
}

GLint GLEScmContext::getErrorCoreProfile() {
    return core().getAndClearLastError();
}

void GLEScmContext::enable(GLenum cap) {
    setEnable(cap, true);

    switch (cap) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
            setTextureEnabled(cap,true);
    }

    if (isCoreProfile()) {
        core().enable(cap);
    } else {
        if (cap==GL_TEXTURE_GEN_STR_OES) {
            dispatcher().glEnable(GL_TEXTURE_GEN_S);
            dispatcher().glEnable(GL_TEXTURE_GEN_T);
            dispatcher().glEnable(GL_TEXTURE_GEN_R);
        } else {
            dispatcher().glEnable(cap);
        }
    }
}

void GLEScmContext::disable(GLenum cap) {
    setEnable(cap, false);

    switch (cap) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
            setTextureEnabled(cap, false);
    }

    if (isCoreProfile()) {
        core().disable(cap);
    } else {
        if (cap==GL_TEXTURE_GEN_STR_OES) {
            dispatcher().glDisable(GL_TEXTURE_GEN_S);
            dispatcher().glDisable(GL_TEXTURE_GEN_T);
            dispatcher().glDisable(GL_TEXTURE_GEN_R);
        } else {
            dispatcher().glDisable(cap);
        }
    }
}

void GLEScmContext::shadeModel(GLenum mode) {
    mShadeModel = mode;

    if (isCoreProfile()) {
        core().shadeModel(mode);
    } else {
        dispatcher().glShadeModel(mode);
    }
}

void GLEScmContext::matrixMode(GLenum mode) {
    mCurrMatrixMode = mode;

    if (isCoreProfile()) {
        core().matrixMode(mode);
    } else {
        dispatcher().glMatrixMode(mode);
    }
}

void GLEScmContext::loadIdentity() {
    currMatrix() = glm::mat4();

    if (isCoreProfile()) {
        core().loadIdentity();
    } else {
        dispatcher().glLoadIdentity();
    }
}

void GLEScmContext::pushMatrix() {
    if (currMatrixStack().size() >= kMaxMatrixStackSize) {
        setGLerror(GL_STACK_OVERFLOW);
        return;
    }
    currMatrixStack().emplace_back(currMatrixStack().back());

    if (isCoreProfile()) {
        core().pushMatrix();
    } else {
        dispatcher().glPushMatrix();
    }
}

void GLEScmContext::popMatrix() {
    if (currMatrixStack().size() == 1) {
        setGLerror(GL_STACK_UNDERFLOW);
        return;
    }
    currMatrixStack().pop_back();

    if (isCoreProfile()) {
        core().popMatrix();
    } else {
        dispatcher().glPopMatrix();
    }
}

void GLEScmContext::multMatrixf(const GLfloat* m) {
    currMatrix() *= glm::make_mat4(m);

    if (isCoreProfile()) {
        core().multMatrixf(m);
    } else {
        dispatcher().glMultMatrixf(m);
    }
}

void GLEScmContext::orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    currMatrix() *= glm::ortho(left, right, bottom, top, zNear, zFar);

    if (isCoreProfile()) {
        core().orthof(left, right, bottom, top, zNear, zFar);
    } else {
        dispatcher().glOrtho(left,right,bottom,top,zNear,zFar);
    }
}

void GLEScmContext::frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    currMatrix() *= glm::frustum(left, right, bottom, top, zNear, zFar);

    if (isCoreProfile()) {
        core().frustumf(left, right, bottom, top, zNear, zFar);
    } else {
        dispatcher().glFrustum(left,right,bottom,top,zNear,zFar);
    }
}

void GLEScmContext::texEnvf(GLenum target, GLenum pname, GLfloat param) {
    // Assume |target| is GL_TEXTURE_ENV
    if (pname == GL_TEXTURE_ENV_MODE) {
        texEnvi(target, pname, (GLint)param);
    } else {
        mTexUnitEnvs[m_activeTexture][pname].floatVal[0] = param;
    }

    if (isCoreProfile()) {
        core().texEnvf(target, pname, param);
    } else {
        dispatcher().glTexEnvf(target, pname, param);
    }
}

void GLEScmContext::texEnvfv(GLenum target, GLenum pname, const GLfloat* params) {
    if (pname == GL_TEXTURE_ENV_COLOR) {
        for (int i = 0; i < 4; i++) {
            mTexUnitEnvs[m_activeTexture][pname].floatVal[i] = params[i];
        }
    } else {
        texEnvf(target, pname, params[0]);
    }

    if (isCoreProfile()) {
        core().texEnvfv(target, pname, params);
    } else {
        dispatcher().glTexEnvfv(target, pname, params);
    }
}

void GLEScmContext::texEnvi(GLenum target, GLenum pname, GLint param) {
    mTexUnitEnvs[m_activeTexture][pname].intVal[0] = param;

    if (isCoreProfile()) {
        core().texEnvi(target, pname, param);
    } else {
        dispatcher().glTexEnvi(target, pname, param);
    }
}

void GLEScmContext::texEnviv(GLenum target, GLenum pname, const GLint* params) {
    mTexUnitEnvs[m_activeTexture][pname].intVal[0] = params[0];

    if (isCoreProfile()) {
        core().texEnviv(target, pname, params);
    } else {
        dispatcher().glTexEnviv(target, pname, params);
    }
}

void GLEScmContext::getTexEnvfv(GLenum env, GLenum pname, GLfloat* params) {
    *params = mTexUnitEnvs[m_activeTexture][pname].floatVal[0];

    if (isCoreProfile()) {
        core().getTexEnvfv(env, pname, params);
    } else {
        dispatcher().glGetTexEnvfv(env, pname, params);
    }
}

void GLEScmContext::getTexEnviv(GLenum env, GLenum pname, GLint* params) {
    *params = mTexUnitEnvs[m_activeTexture][pname].intVal[0];

    if (isCoreProfile()) {
        core().getTexEnviv(env, pname, params);
    } else {
        dispatcher().glGetTexEnviv(env, pname, params);
    }
}

void GLEScmContext::texGenf(GLenum coord, GLenum pname, GLfloat param) {
    mTexGens[m_activeTexture][pname].floatVal[0] = param;

    if (isCoreProfile()) {
        core().texGenf(coord, pname, param);
    } else {
        if (coord == GL_TEXTURE_GEN_STR_OES) {
            dispatcher().glTexGenf(GL_S,pname,param);
            dispatcher().glTexGenf(GL_T,pname,param);
            dispatcher().glTexGenf(GL_R,pname,param);
        } else {
            dispatcher().glTexGenf(coord,pname,param);
        }
    }
}

void GLEScmContext::texGenfv(GLenum coord, GLenum pname, const GLfloat* params) {
    mTexGens[m_activeTexture][pname].floatVal[0] = params[0];

    if (isCoreProfile()) {
        core().texGenfv(coord, pname, params);
    } else {
        if (coord == GL_TEXTURE_GEN_STR_OES) {
            dispatcher().glTexGenfv(GL_S,pname,params);
            dispatcher().glTexGenfv(GL_T,pname,params);
            dispatcher().glTexGenfv(GL_R,pname,params);
        } else {
            dispatcher().glTexGenfv(coord,pname,params);
        }
    }
}

void GLEScmContext::texGeni(GLenum coord, GLenum pname, GLint param) {
    mTexGens[m_activeTexture][pname].intVal[0] = param;

    if (isCoreProfile()) {
        core().texGeni(coord, pname, param);
    } else {
        if (coord == GL_TEXTURE_GEN_STR_OES) {
            dispatcher().glTexGeni(GL_S,pname,param);
            dispatcher().glTexGeni(GL_T,pname,param);
            dispatcher().glTexGeni(GL_R,pname,param);
        } else {
            dispatcher().glTexGeni(coord,pname,param);
        }
    }
}

void GLEScmContext::texGeniv(GLenum coord, GLenum pname, const GLint* params) {
    mTexGens[m_activeTexture][pname].intVal[0] = params[0];

    if (isCoreProfile()) {
        core().texGeniv(coord, pname, params);
    } else {
        if (coord == GL_TEXTURE_GEN_STR_OES) {
            dispatcher().glTexGeniv(GL_S,pname,params);
            dispatcher().glTexGeniv(GL_T,pname,params);
            dispatcher().glTexGeniv(GL_R,pname,params);
        } else {
            dispatcher().glTexGeniv(coord,pname,params);
        }
    }
}

void GLEScmContext::getTexGeniv(GLenum coord, GLenum pname, GLint* params) {
    *params = mTexGens[m_activeTexture][pname].intVal[0];

    if (isCoreProfile()) {
        core().getTexGeniv(coord, pname, params);
    } else {
        if (coord == GL_TEXTURE_GEN_STR_OES) {
            GLint state_s = GL_FALSE;
            GLint state_t = GL_FALSE;
            GLint state_r = GL_FALSE;
            dispatcher().glGetTexGeniv(GL_S,pname,&state_s);
            dispatcher().glGetTexGeniv(GL_T,pname,&state_t);
            dispatcher().glGetTexGeniv(GL_R,pname,&state_r);
            *params = state_s && state_t && state_r ? GL_TRUE: GL_FALSE;
        } else {
            dispatcher().glGetTexGeniv(coord,pname,params);
        }
    }
}

void GLEScmContext::getTexGenfv(GLenum coord, GLenum pname, GLfloat* params) {
    params[0] = mTexGens[m_activeTexture][pname].floatVal[0];
    params[1] = mTexGens[m_activeTexture][pname].floatVal[1];
    params[2] = mTexGens[m_activeTexture][pname].floatVal[2];
    params[3] = mTexGens[m_activeTexture][pname].floatVal[3];

    if (isCoreProfile()) {
        core().getTexGenfv(coord, pname, params);
    } else {
        if (coord == GL_TEXTURE_GEN_STR_OES) {
            GLfloat state_s = GL_FALSE;
            GLfloat state_t = GL_FALSE;
            GLfloat state_r = GL_FALSE;
            dispatcher().glGetTexGenfv(GL_S,pname,&state_s);
            dispatcher().glGetTexGenfv(GL_T,pname,&state_t);
            dispatcher().glGetTexGenfv(GL_R,pname,&state_r);
            *params = state_s && state_t && state_r ? GL_TRUE: GL_FALSE;
        } else {
            dispatcher().glGetTexGenfv(coord,pname,params);
        }
    }
}

void GLEScmContext::enableClientState(GLenum clientState) {
    // TODO: Track enabled state in vao state.
    if (isCoreProfile()) {
        core().enableClientState(clientState);
    } else {
        dispatcher().glEnableClientState(clientState);
    }
}

void GLEScmContext::disableClientState(GLenum clientState) {
    // TODO: Track enabled state in vao state.
    if (isCoreProfile()) {
        core().disableClientState(clientState);
    } else {
        dispatcher().glDisableClientState(clientState);
    }
}

void GLEScmContext::drawTexOES(float x, float y, float z, float width, float height) {
    if (isCoreProfile()) {
        core().drawTexOES(x, y, z, width, height);
    } else {
        auto& gl = dispatcher();

        int numClipPlanes;

        GLint viewport[4] = {};
        z = (z>1 ? 1 : (z<0 ?  0 : z));

        float vertices[4*3] = {
            x , y, z,
            x , static_cast<float>(y+height), z,
            static_cast<float>(x+width), static_cast<float>(y+height), z,
            static_cast<float>(x+width), y, z
        };
        GLfloat texels[getMaxTexUnits()][4*2];
        memset((void*)texels, 0, getMaxTexUnits()*4*2*sizeof(GLfloat));

        gl.glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        gl.glPushAttrib(GL_TRANSFORM_BIT);

        //setup projection matrix to draw in viewport aligned coordinates
        gl.glMatrixMode(GL_PROJECTION);
        gl.glPushMatrix();
        gl.glLoadIdentity();
        gl.glGetIntegerv(GL_VIEWPORT,viewport);
        gl.glOrtho(viewport[0],viewport[0] + viewport[2],viewport[1],viewport[1]+viewport[3],0,-1);
        //setup texture matrix
        gl.glMatrixMode(GL_TEXTURE);
        gl.glPushMatrix();
        gl.glLoadIdentity();
        //setup modelview matrix
        gl.glMatrixMode(GL_MODELVIEW);
        gl.glPushMatrix();
        gl.glLoadIdentity();
        //backup vbo's
        int array_buffer,element_array_buffer;
        gl.glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&array_buffer);
        gl.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&element_array_buffer);
        gl.glBindBuffer(GL_ARRAY_BUFFER,0);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

        //disable clip planes
        gl.glGetIntegerv(GL_MAX_CLIP_PLANES,&numClipPlanes);
        for (int i=0;i<numClipPlanes;++i)
            gl.glDisable(GL_CLIP_PLANE0+i);

        int nTexPtrs = 0;
        for (int i=0;i<getMaxTexUnits();++i) {
            if (isTextureUnitEnabled(GL_TEXTURE0+i)) {
                TextureData * texData = NULL;
                unsigned int texname = getBindedTexture(GL_TEXTURE0+i,GL_TEXTURE_2D);
                ObjectLocalName tex = getTextureLocalName(GL_TEXTURE_2D,texname);
                gl.glClientActiveTexture(GL_TEXTURE0+i);
                auto objData = shareGroup()->getObjectData(
                        NamedObjectType::TEXTURE, tex);
                if (objData) {
                    texData = (TextureData*)objData;
                    //calculate texels
                    texels[i][0] = (float)(texData->crop_rect[0])/(float)(texData->width);
                    texels[i][1] = (float)(texData->crop_rect[1])/(float)(texData->height);

                    texels[i][2] = (float)(texData->crop_rect[0])/(float)(texData->width);
                    texels[i][3] = (float)(texData->crop_rect[3]+texData->crop_rect[1])/(float)(texData->height);

                    texels[i][4] = (float)(texData->crop_rect[2]+texData->crop_rect[0])/(float)(texData->width);
                    texels[i][5] = (float)(texData->crop_rect[3]+texData->crop_rect[1])/(float)(texData->height);

                    texels[i][6] = (float)(texData->crop_rect[2]+texData->crop_rect[0])/(float)(texData->width);
                    texels[i][7] = (float)(texData->crop_rect[1])/(float)(texData->height);

                    gl.glTexCoordPointer(2,GL_FLOAT,0,texels[i]);
                    nTexPtrs++;
                }
            }
        }

        if (nTexPtrs>0) {
            //draw rectangle - only if we have some textures enabled & ready
            gl.glEnableClientState(GL_VERTEX_ARRAY);
            gl.glVertexPointer(3,GL_FLOAT,0,vertices);
            gl.glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            gl.glDrawArrays(GL_TRIANGLE_FAN,0,4);
        }

        //restore vbo's
        gl.glBindBuffer(GL_ARRAY_BUFFER,array_buffer);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,element_array_buffer);

        //restore matrix state

        gl.glMatrixMode(GL_MODELVIEW);
        gl.glPopMatrix();
        gl.glMatrixMode(GL_TEXTURE);
        gl.glPopMatrix();
        gl.glMatrixMode(GL_PROJECTION);
        gl.glPopMatrix();

        gl.glPopAttrib();
        gl.glPopClientAttrib();
    }
}

void GLEScmContext::rotatef(float angle, float x, float y, float z) {
    glm::mat4 rot = glm::rotate(glm::mat4(), 3.14159265358979f / 180.0f * angle, glm::vec3(x, y, z));
    currMatrix() *= rot;

    if (isCoreProfile()) {
        core().rotatef(angle, x, y, z);
    } else {
        dispatcher().glRotatef(angle, x, y, z);
    }
}

void GLEScmContext::scalef(float x, float y, float z) {
    glm::mat4 scale = glm::scale(glm::mat4(), glm::vec3(x, y, z));
    currMatrix() *= scale;

    if (isCoreProfile()) {
        core().scalef(x, y, z);
    } else {
        dispatcher().glScalef(x, y, z);
    }
}

void GLEScmContext::translatef(float x, float y, float z) {
    glm::mat4 tr = glm::translate(glm::mat4(), glm::vec3(x, y, z));
    currMatrix() *= tr;

    if (isCoreProfile()) {
        core().translatef(x, y, z);
    } else {
        dispatcher().glTranslatef(x, y, z);
    }
}

void GLEScmContext::color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {

    mColor.type = GL_FLOAT;
    mColor.val.floatVal[0] = red;
    mColor.val.floatVal[1] = green;
    mColor.val.floatVal[2] = blue;
    mColor.val.floatVal[3] = alpha;

    if (isCoreProfile()) {
        core().color4f(red,green,blue,alpha);
    } else{
        dispatcher().glColor4f(red,green,blue,alpha);
    }
}

void GLEScmContext::color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {

    mColor.type = GL_UNSIGNED_BYTE;
    mColor.val.ubyteVal[0] = red;
    mColor.val.ubyteVal[1] = green;
    mColor.val.ubyteVal[2] = blue;
    mColor.val.ubyteVal[3] = alpha;

    if (isCoreProfile()) {
        core().color4ub(red,green,blue,alpha);
    } else{
        dispatcher().glColor4ub(red,green,blue,alpha);
    }
}

void GLEScmContext::drawArrays(GLenum mode, GLint first, GLsizei count) {
    if (!isArrEnabled(GL_VERTEX_ARRAY)) return;

    drawValidate();

    if (isCoreProfile()) {
        ArraysMap::iterator it;
        m_pointsIndex = -1;

        // going over all clients arrays Pointers
        for (it = m_currVaoState.begin();
             it != m_currVaoState.end(); ++it) {
            GLenum array_id = (*it).first;
            GLESpointer* p  = (*it).second;
            if (array_id == GL_VERTEX_ARRAY ||
                array_id == GL_NORMAL_ARRAY ||
                array_id == GL_COLOR_ARRAY ||
                array_id == GL_POINT_SIZE_ARRAY_OES ||
                array_id == GL_TEXTURE_COORD_ARRAY) {
                core().setupArrayForDraw(array_id, p, first, count, false, 0, nullptr);
            }
        }

        GLenum activeTexture = m_clientActiveTexture + GL_TEXTURE0;
        setClientActiveTexture(activeTexture);
        core().clientActiveTexture(activeTexture);
        core().drawArrays(mode, first, count);
    } else {
        GLESConversionArrays tmpArrs;

        setupArraysPointers(tmpArrs,first,count,0,NULL,true);

        if (mode == GL_POINTS && isArrEnabled(GL_POINT_SIZE_ARRAY_OES)){
            drawPointsArrs(tmpArrs,first,count);
        } else {
            dispatcher().glDrawArrays(mode,first,count);
        }
    }
}

void GLEScmContext::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    if (!isArrEnabled(GL_VERTEX_ARRAY)) return;

    drawValidate();

    if(isBindedBuffer(GL_ELEMENT_ARRAY_BUFFER)) { // if vbo is binded take the indices from the vbo
        const unsigned char* buf = static_cast<unsigned char *>(getBindedBuffer(GL_ELEMENT_ARRAY_BUFFER));
        indices = buf + SafeUIntFromPointer(indices);
    }

    if (isCoreProfile()) {
        ArraysMap::iterator it;
        m_pointsIndex = -1;

        // going over all clients arrays Pointers
        for (it = m_currVaoState.begin();
             it != m_currVaoState.end(); ++it) {
            GLenum array_id = (*it).first;
            GLESpointer* p  = (*it).second;
            if (array_id == GL_VERTEX_ARRAY ||
                array_id == GL_NORMAL_ARRAY ||
                array_id == GL_COLOR_ARRAY ||
                array_id == GL_POINT_SIZE_ARRAY_OES ||
                array_id == GL_TEXTURE_COORD_ARRAY) {
                core().setupArrayForDraw(array_id, p, 0, count, true, type, indices);
            }
        }

        GLenum activeTexture = m_clientActiveTexture + GL_TEXTURE0;
        setClientActiveTexture(activeTexture);
        core().clientActiveTexture(activeTexture);
        core().drawElements(mode, count, type, indices);
    } else {
        GLESConversionArrays tmpArrs;

        setupArraysPointers(tmpArrs,0,count,type,indices,false);
        if(mode == GL_POINTS && isArrEnabled(GL_POINT_SIZE_ARRAY_OES)){
            drawPointsElems(tmpArrs,count,type,indices);
        }
        else{
            dispatcher().glDrawElements(mode,count,type,indices);
        }
    }
}

void GLEScmContext::clientActiveTexture(GLenum texture) {
    if (isCoreProfile()) {
        core().clientActiveTexture(texture);
    } else {
        dispatcher().glClientActiveTexture(texture);
    }
}
