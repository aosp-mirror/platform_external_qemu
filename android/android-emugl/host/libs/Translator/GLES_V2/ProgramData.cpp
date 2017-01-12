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
#include <GLES3/gl31.h>
#include <GLcommon/objectNameManager.h>
#include "ProgramData.h"

ProgramData::ProgramData() :  ObjectData(PROGRAM_DATA),
                              AttachedVertexShader(0),
                              AttachedFragmentShader(0),
                              AttachedComputeShader(0),
                              LinkStatus(GL_FALSE),
                              IsInUse(false),
                              DeleteStatus(false) {}

void ProgramData::setInfoLog(const GLchar* log) {
    infoLog.reset(log);
}

const GLchar* ProgramData::getInfoLog() const {
    return infoLog.get() ? infoLog.get() : (const GLchar*)"";
}

GLuint ProgramData::getAttachedVertexShader() const {
    return AttachedVertexShader;
}

GLuint ProgramData::getAttachedFragmentShader() const {
    return AttachedFragmentShader;
}

GLuint ProgramData::getAttachedComputeShader() const {
    return AttachedComputeShader;
}

GLuint ProgramData::getAttachedShader(GLenum type) const {
    GLuint shader = 0;
    switch (type) {
    case GL_VERTEX_SHADER:
        shader = AttachedVertexShader;
        break;
    case GL_FRAGMENT_SHADER:
        shader = AttachedFragmentShader;
        break;
    case GL_COMPUTE_SHADER:
        shader = AttachedComputeShader;
        break;
    }
    return shader;
}

bool ProgramData::attachShader(GLuint shader,GLenum type) {
    if (type==GL_VERTEX_SHADER && AttachedVertexShader==0) {
        AttachedVertexShader=shader;
        return true;
    }
    else if (type==GL_FRAGMENT_SHADER && AttachedFragmentShader==0) {
        AttachedFragmentShader=shader;
        return true;
    }
    else if (type==GL_COMPUTE_SHADER && AttachedComputeShader==0) {
        AttachedComputeShader=shader;
        return true;
    }
    return false;
}

bool ProgramData::isAttached(GLuint shader) const {
    return AttachedFragmentShader == shader ||
           AttachedVertexShader == shader ||
           AttachedComputeShader == shader;
}

bool ProgramData::detachShader(GLuint shader) {
    if (AttachedVertexShader==shader) {
        AttachedVertexShader = 0;
        return true;
    }
    else if (AttachedFragmentShader==shader) {
        AttachedFragmentShader = 0;
        return true;
    }
    else if (AttachedComputeShader==shader) {
        AttachedComputeShader = 0;
        return true;
    }
    return false;
}

void ProgramData::setLinkStatus(GLint status) {
    LinkStatus = status;
}

GLint ProgramData::getLinkStatus() const {
    return LinkStatus;
}
