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

#pragma once

#include "GLcommon/ObjectData.h"

#include <vector>

class BufferBinding;

class TransformFeedbackData : public ObjectData {
public:
    TransformFeedbackData() : ObjectData(TRANSFORMFEEDBACK_DATA) {}
    TransformFeedbackData(android::base::Stream* stream);
    void setMaxSize(int maxSize);
    void onSave(android::base::Stream* stream,
                unsigned int globalName) const override;
    void restore(ObjectLocalName localName,
                 const getGlobalName_t& getGlobalName) override;
    void bindIndexedBuffer(GLuint index,
                           GLuint buffer,
                           GLintptr offset,
                           GLsizeiptr size,
                           GLintptr stride,
                           bool isBindBase);
    GLuint getIndexedBuffer(GLuint index);
    void unbindBuffer(GLuint buffer);
    bool mIsActive = false;

private:
    std::vector<BufferBinding> m_indexedTransformFeedbackBuffers;
};
