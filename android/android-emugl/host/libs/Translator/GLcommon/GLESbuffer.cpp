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
#include <GLcommon/GLESbuffer.h>
#include <GLcommon/GLEScontext.h>
#include <string.h>

bool  GLESbuffer::setBuffer(GLuint size,GLuint usage,const GLvoid* data) {
    m_size = size;
    m_usage = usage;
    if(m_data) {
        delete [] m_data;
        m_data = NULL;
    }
    m_data = new unsigned char[size];
    if(m_data) {
        if(data) {
            memcpy(m_data,data,size);
        }
        m_conversionManager.clear();
        m_conversionManager.addRange(Range(0,m_size));
        return true;
    }
    return false;
}

bool  GLESbuffer::setSubBuffer(GLint offset,GLuint size,const GLvoid* data) {
    if(offset + size > m_size) return false;
    memcpy(m_data+offset,data,size);
    m_conversionManager.addRange(Range(offset,size));
    m_conversionManager.merge();
    return true;
}

void  GLESbuffer::getConversions(const RangeList& rIn,RangeList& rOut) {
        m_conversionManager.delRanges(rIn,rOut);
        rOut.merge();
}

GLESbuffer::~GLESbuffer() {
    if(m_data) {
        delete [] m_data;
    }
}

GLESbuffer::GLESbuffer(android::base::Stream* stream) : ObjectData(stream) {
    m_size = stream->getBe32();
    m_usage = stream->getBe32();
    if (m_size) {
        m_data = new unsigned char[m_size];
        stream->read(m_data, m_size);
        // TODO: m_conversionManager loading
        m_conversionManager.addRange(Range(0,m_size));
    }
    m_wasBound = stream->getByte();
}

void GLESbuffer::onSave(android::base::Stream* stream,
        unsigned int globalName) const {
    ObjectData::onSave(stream, globalName);
    stream->putBe32(m_size);
    stream->putBe32(m_usage);
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    bool mapSuccess = false;
    if (!needRestore() && dispatcher.glMapBufferRange && m_size != 0) {
        // If glMapBufferRange is supported, m_data might be inconsistent
        // with GPU memory (because we did not update m_data when glUnmapBuffer)
        // Thus we directly load buffer data from GPU memory

        // + We do not handle the situation when snapshot happens between
        // a map and an unmap, because the way we implement GLES decoder
        // prevents such behavior.
        int prevBuffer = 0;
        dispatcher.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevBuffer);
        dispatcher.glBindBuffer(GL_ARRAY_BUFFER, globalName);
        void * data = dispatcher.glMapBufferRange(GL_ARRAY_BUFFER, 0,
                    m_size, GL_MAP_READ_BIT);
        assert(data);
        // BUG: 68051848
        // It is supposed to be fixed, but for safety we keep the fallback path
        // here.
        if (data) {
            stream->write(data, m_size);
            bool success = dispatcher.glUnmapBuffer(GL_ARRAY_BUFFER);
            assert(success);
            (void)success;
            mapSuccess = true;
        }
        dispatcher.glBindBuffer(GL_ARRAY_BUFFER, prevBuffer);
    }
    if (!mapSuccess) {
        stream->write(m_data, m_size);
    }

    // TODO: m_conversionManager
    //
    // Treat it as a low priority issue for now because in GLES2 this is only
    // used for fix point vertex buffers. We are very unlikely to hit it
    // when snapshotting home screen.
    stream->putByte(m_wasBound);
}

void GLESbuffer::restore(ObjectLocalName localName,
        const getGlobalName_t& getGlobalName) {
    ObjectData::restore(localName, getGlobalName);
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    int globalName = getGlobalName(NamedObjectType::VERTEXBUFFER, localName);
    // We bind to GL_ARRAY_BUFFER just for uploading buffer data
    dispatcher.glBindBuffer(GL_ARRAY_BUFFER, globalName);
    dispatcher.glBufferData(GL_ARRAY_BUFFER, m_size, m_data, m_usage);
}
