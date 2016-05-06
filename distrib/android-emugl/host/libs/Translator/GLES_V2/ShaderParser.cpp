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

#include "ShaderParser.h"
#include "ANGLEShaderParser.h"
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <string>
#include <vector>

ShaderParser::ShaderParser(GLenum type):ObjectData(SHADER_DATA), m_type(type) {}

void ShaderParser::convertESSLToGLSL(const char* src) {
    std::string infolog;
    std::string parsedSource;
    m_valid = ANGLEShaderParser::translate(src, m_type, &infolog, &parsedSource);

    if (!m_valid) {
        unsigned len = infolog.length();
        GLchar* log = new GLchar[len];
        memcpy(log, infolog.c_str(), len);
        setInfoLog(log);
    } else {
        m_parsedSrc = parsedSource;
    }
}

void ShaderParser::setSrc(const Version& ver, GLsizei count, const GLchar* const* strings, const GLint* length){
    m_originalSrc.clear();
    for(int i = 0;i<count;i++){
        const size_t strLen =
                (length && length[i] >= 0) ? length[i] : strlen(strings[i]);
        m_originalSrc.append(strings[i], strings[i] + strLen);
    }

    clearParsedSrc();
    convertESSLToGLSL(m_originalSrc.c_str());
}

const GLchar** ShaderParser::parsedLines() {
      m_parsedLines = (GLchar*)m_parsedSrc.c_str();
      return const_cast<const GLchar**> (&m_parsedLines);
}

void ShaderParser::clear() {
    m_parsedLines = nullptr;
    std::string().swap(m_parsedSrc);
}

const std::string& ShaderParser::getOriginalSrc() const {
    return m_originalSrc;
}

void ShaderParser::clearParsedSrc(){
    m_parsedSrc.clear();
}

GLenum ShaderParser::getType() {
    return m_type;
}

void ShaderParser::setInfoLog(GLchar* infoLog) {
    assert(infoLog);
    std::unique_ptr<GLchar[]> infoLogDeleter(infoLog);
    m_infoLog.assign(infoLog);
}

bool ShaderParser::validShader() const {
    return m_valid;
}

const GLchar* ShaderParser::getInfoLog() const {
    return m_infoLog.c_str();
}
