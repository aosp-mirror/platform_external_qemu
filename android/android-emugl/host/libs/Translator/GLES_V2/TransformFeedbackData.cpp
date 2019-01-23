/*
* Copyright (C) 2019 The Android Open Source Project
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

#include "TransformFeedbackData.h"

#include "GLcommon/GLEScontext.h"
#include "GLcommon/GLSnapshotSerializers.h"

TransformFeedbackData::TransformFeedbackData(android::base::Stream* stream) {
    if (stream) {
        loadContainer(stream, m_indexedTransformFeedbackBuffers);
    }
}

void TransformFeedbackData::setMaxSize(int maxSize) {
    m_indexedTransformFeedbackBuffers.resize(maxSize);
}

void TransformFeedbackData::onSave(android::base::Stream* stream,
                unsigned int globalName) const {
    saveContainer(stream, m_indexedTransformFeedbackBuffers);            
}

void TransformFeedbackData::restore(ObjectLocalName localName,
                 const getGlobalName_t& getGlobalName) {
    // TODO
}

void TransformFeedbackData::bindIndexedBuffer(GLuint index, GLuint buffer,
        GLintptr offset, GLsizeiptr size, GLintptr stride, bool isBindBase) {
    if (index >= m_indexedTransformFeedbackBuffers.size()) {
            return;
    }
    auto& bufferBinding = m_indexedTransformFeedbackBuffers[index];
    bufferBinding.buffer = buffer;
    bufferBinding.offset = offset;
    bufferBinding.size = size;
    bufferBinding.stride = stride;
    bufferBinding.isBindBase = isBindBase;
}

void TransformFeedbackData::unbindBuffer(GLuint buffer) {
    for (auto& bufferBinding : m_indexedTransformFeedbackBuffers) {
        if (bufferBinding.buffer == buffer) {
            bufferBinding = {};
        }
    }
}

GLuint TransformFeedbackData::getIndexedBuffer(GLuint index) {
    return m_indexedTransformFeedbackBuffers[index].buffer;
}
