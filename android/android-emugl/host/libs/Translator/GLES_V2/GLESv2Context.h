/*
* Copyright 2011 The Android Open Source Project
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

#ifndef GLES_V2_CONTEXT_H
#define GLES_V2_CONTEXT_H

#include <GLcommon/GLDispatch.h>
#include <GLcommon/GLEScontext.h>
#include <GLcommon/objectNameManager.h>

class GLESv2Context : public GLEScontext{
public:
    virtual void init(GlLibrary* glLib);
    GLESv2Context(int maj, int min);
    virtual ~GLESv2Context();
    void setupArraysPointers(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct);
    void setVertexAttribDivisor(GLuint bindingindex, GLuint divisor);
    void setVertexAttribBindingIndex(GLuint attribindex, GLuint bindingindex);
    void setVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint reloffset, bool isInt = false);
    int  getMaxCombinedTexUnits() override;
    int  getMaxTexUnits() override;

    // This whole att0 thing is about a incompatibility between GLES and OpenGL.
    // GLES allows a vertex shader attribute to be in location 0 and have a
    // current value, while OpenGL is not very clear about this, which results
    // in each implementation doing something different.
    void setAttribute0value(float x, float y, float z, float w);
    bool needAtt0PreDrawValidation();
    void validateAtt0PreDraw(unsigned int count);
    void validateAtt0PostDraw(void);
    const float* getAtt0(void) const {return m_attribute0value;}

protected:
    bool needConvert(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id);
private:
    void setupArr(const GLvoid* arr,GLenum arrayType,GLenum dataType,GLint size,GLsizei stride,GLboolean normalized, int pointsIndex = -1, bool isInt = false);
    void initExtensionString();

    float m_attribute0value[4] = {};
    bool m_attribute0valueChanged = true;
    GLfloat* m_att0Array = nullptr;
    unsigned int m_att0ArrayLength = 0;
    bool m_att0NeedsDisable = false;
};

#endif
