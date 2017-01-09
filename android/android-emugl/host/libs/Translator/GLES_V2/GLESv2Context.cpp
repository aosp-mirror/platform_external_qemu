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

#include "GLESv2Context.h"

#include <string.h>

static const char kGLES20StringPart[] = "OpenGL ES 2.0";
static const char kGLES30StringPart[] = "OpenGL ES 3.0";
static const char kGLES31StringPart[] = "OpenGL ES 3.1";
static const char kGLES32StringPart[] = "OpenGL ES 3.2";

static const char* sPickVersionStringPart(int maj, int min) {
    switch (maj) {
        case 2:
            return kGLES20StringPart;
        case 3:
            switch (min) {
                case 0:
                    return kGLES30StringPart;
                case 1:
                    return kGLES31StringPart;
                case 2:
                    return kGLES32StringPart;
                default:
                    return nullptr;
            }
        default:
            return nullptr;
    }
    return nullptr;
}

void GLESv2Context::init(GlLibrary* glLib) {
    emugl::Mutex::AutoLock mutex(s_lock);
    if(!m_initialized) {
        s_glDispatch.dispatchFuncs(GLES_2_0, glLib);
        GLEScontext::init(glLib);
        addVertexArrayObject(0);
        setVertexArrayObject(0);
        setAttribute0value(0.0, 0.0, 0.0, 1.0);

        buildStrings((const char*)dispatcher().glGetString(GL_VENDOR),
                     (const char*)dispatcher().glGetString(GL_RENDERER),
                     (const char*)dispatcher().glGetString(GL_VERSION),
                     sPickVersionStringPart(m_glesMajorVersion, m_glesMinorVersion));
    }
    m_initialized = true;
}

GLESv2Context::GLESv2Context(int maj, int min) {
    m_glesMajorVersion = maj;
    m_glesMinorVersion = min;
}

GLESv2Context::~GLESv2Context()
{
    delete[] m_att0Array;
}

void GLESv2Context::setAttribute0value(float x, float y, float z, float w)
{
    m_attribute0valueChanged |=
            x != m_attribute0value[0] || y != m_attribute0value[1] ||
            z != m_attribute0value[2] || w != m_attribute0value[3];
    m_attribute0value[0] = x;
    m_attribute0value[1] = y;
    m_attribute0value[2] = z;
    m_attribute0value[3] = w;
}

bool GLESv2Context::needAtt0PreDrawValidation()
{
    m_att0NeedsDisable = false;

    int enabled = 0;
    s_glDispatch.glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    return enabled == 0;
}

void GLESv2Context::validateAtt0PreDraw(unsigned int count)
{
    if (count == 0) {
        return;
    }

    if (count > m_att0ArrayLength) {
        delete [] m_att0Array;
        const unsigned newLen = std::max(count, 2 * m_att0ArrayLength);
        m_att0Array = new GLfloat[4 * newLen];
        m_att0ArrayLength = newLen;
        m_attribute0valueChanged = true;
    }
    if (m_attribute0valueChanged) {
        for(unsigned int i = 0; i<m_att0ArrayLength; i++) {
            memcpy(m_att0Array+i*4, m_attribute0value, sizeof(m_attribute0value));
        }
        m_attribute0valueChanged = false;
    }

    s_glDispatch.glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, m_att0Array);
    s_glDispatch.glEnableVertexAttribArray(0);

    m_att0NeedsDisable = true;
}

void GLESv2Context::validateAtt0PostDraw(void)
{
    if (m_att0NeedsDisable) {
        s_glDispatch.glDisableVertexAttribArray(0);
        m_att0NeedsDisable = false;
    }
}

void GLESv2Context::setupArraysPointers(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct) {
    ArraysMap::iterator it;

    //going over all clients arrays Pointers
    for ( it=m_currVaoState.begin() ; it != m_currVaoState.end(); ++it) {
        GLenum array_id = (*it).first;
        GLESpointer* p = (*it).second;
        if (!p->isEnable()) continue;

        setupArr(p->getData(),
                 array_id,
                 p->getType(),
                 p->getSize(),
                 p->getStride(),
                 p->getNormalized(),
                 -1,
                 p->isIntPointer());
    }
}

//setting client side arr
void GLESv2Context::setupArr(const GLvoid* arr,GLenum arrayType,GLenum dataType,GLint size,GLsizei stride,GLboolean normalized, int index, bool isInt){
    // is not really a client side arr.
    if (arr == NULL) return;

    if (isInt)
        s_glDispatch.glVertexAttribIPointer(arrayType, size, dataType, stride, arr);
    else
        s_glDispatch.glVertexAttribPointer(arrayType, size, dataType, normalized, stride, arr);
}

void GLESv2Context::setVertexAttribDivisor(GLuint bindingindex, GLuint divisor) {
    m_currVaoState.bufferBindings()[bindingindex].divisor = divisor;
}

void GLESv2Context::setVertexAttribBindingIndex(GLuint attribindex, GLuint bindingindex) {
    m_currVaoState[attribindex]->setBindingIndex(bindingindex);
}

void GLESv2Context::setVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint reloffset, bool isInt) {
    m_currVaoState[attribindex]->setFormat(size, type, normalized == GL_TRUE ? true : false, reloffset, isInt);
}


bool GLESv2Context::needConvert(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id) {

    bool usingVBO = p->isVBO();
    GLenum arrType = p->getType();

    /*
     conversion is not necessary in the following cases:
      (*) array type is not fixed
    */
    if(arrType != GL_FIXED) return false;

    if(!usingVBO) {
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

void GLESv2Context::initExtensionString() {
    *s_glExtensions = "GL_OES_EGL_image GL_OES_EGL_image_external GL_OES_depth24 GL_OES_depth32 GL_OES_element_index_uint "
                      "GL_OES_texture_float GL_OES_texture_float_linear "
                      "GL_OES_compressed_paletted_texture GL_OES_compressed_ETC1_RGB8_texture GL_OES_depth_texture ";
    if (s_glSupport.GL_ARB_HALF_FLOAT_PIXEL || s_glSupport.GL_NV_HALF_FLOAT)
        *s_glExtensions+="GL_OES_texture_half_float GL_OES_texture_half_float_linear ";
    if (s_glSupport.GL_EXT_PACKED_DEPTH_STENCIL)
        *s_glExtensions+="GL_OES_packed_depth_stencil ";
    if (s_glSupport.GL_ARB_HALF_FLOAT_VERTEX)
        *s_glExtensions+="GL_OES_vertex_half_float ";
    if (s_glSupport.GL_OES_STANDARD_DERIVATIVES)
        *s_glExtensions+="GL_OES_standard_derivatives ";
    if (s_glSupport.GL_OES_TEXTURE_NPOT)
        *s_glExtensions+="GL_OES_texture_npot ";
    if (s_glSupport.GL_OES_RGB8_RGBA8) {
        *s_glExtensions+="GL_OES_rgb8_rgba8 ";
    }
}

int GLESv2Context::getMaxTexUnits() {
    return getCaps()->maxTexImageUnits;
}

int GLESv2Context::getMaxCombinedTexUnits() {
    return getCaps()->maxCombinedTexImageUnits;
}
