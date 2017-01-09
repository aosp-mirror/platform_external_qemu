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
#ifndef GLES_POINTER_H
#define GLES_POINTER_H

#include <GLES/gl.h>
#include "GLESbuffer.h"

class GLESpointer {
public:
    GLenum getType() const;
    GLint getSize() const;
    GLsizei getStride() const;
    // get*Data, getBufferName all only for legacy (GLES_CM or buffer->client array converseion) setups.
    const GLvoid* getArrayData() const;
    GLvoid* getBufferData() const;
    GLuint getBufferName() const;
    GLboolean getNormalized() const { return m_normalize ? GL_TRUE : GL_FALSE; }
    const GLvoid* getData() const;
    unsigned int getBufferOffset() const;
    void redirectPointerData();
    void getBufferConversions(const RangeList& rl, RangeList& rlOut);
    bool bufferNeedConversion() { return !m_buffer->fullyConverted(); }
    void setArray(GLint size,
                  GLenum type,
                  GLsizei stride,
                  const GLvoid* data,
                  bool normalize = false,
                  bool isInt = false);
    void setBuffer(GLint size,
                   GLenum type,
                   GLsizei stride,
                   GLESbuffer* buf,
                   GLuint bufferName,
                   int offset,
                   bool normalize = false,
                   bool isInt = false);
    void setDivisor(GLuint divisor);
    void setBindingIndex(GLuint bindingindex);
    void setFormat(GLint size, GLenum type,
                   bool normalize,
                   GLuint reloffset,
                   bool isInt);
    GLuint getBindingIndex() const {
        return m_bindingIndex;
    }
    bool isEnable() const;
    bool isNormalize() const;
    bool isVBO() const;
    bool isIntPointer() const;
    void enable(bool b);

private:
    GLint m_size = 4;
    GLenum m_type = GL_FLOAT;
    GLsizei m_stride = 0;
    bool m_enabled = false;
    bool m_normalize = false;
    bool m_isVBO = false;
    const GLvoid* m_data = nullptr;
    GLESbuffer* m_buffer = nullptr;
    GLuint m_bufferName = 0;
    unsigned int m_buffOffset = 0;
    bool m_isInt = false;
    GLuint m_divisor = 0;
    GLuint m_bindingIndex = 0;
    GLuint m_reloffset = 0;
};
#endif
