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

#ifndef SHADER_PARSER_H
#define SHADER_PARSER_H

#include "GLESv2Context.h"
#include <string>
#include <GLES2/gl2.h>
#include <GLcommon/objectNameManager.h>

class ShaderParser:public ObjectData{
public:
    ShaderParser();
    ShaderParser(GLenum type);
    void           setSrc(const Version& ver,GLsizei count,const GLchar* const* strings,const GLint* length);
    const char*    getOriginalSrc();
    const GLchar** parsedLines();
    GLenum         getType();
    ~ShaderParser();

    void setInfoLog(GLchar * infoLog);
    // Query whether the shader parsed is valid.
    // Don't trust the value if we did not call setSrc
    bool validShader() const;
    // If validation fails, add proper error messages
    // to the parser's info log, which is treated
    // as the actual info log from guest POV.
    void setInvalidInfoLog();

    GLchar* getInfoLog();

    void setDeleteStatus(bool val) { m_deleteStatus = val; }
    bool getDeleteStatus() const { return m_deleteStatus; }

    void setAttachedProgram(GLuint program) { m_program = program; }
    GLuint getAttachedProgram() const { return m_program; }
private:
    // For shader validation purposes, analyze keywords like lowp/highp
    // appearing in variable declarations or function parameters.
    void validateGLESKeywords(const char* src);

    void parseOriginalSrc();
    void parseGLSLversion();
    void parseBuiltinConstants();
    void parseOmitPrecision();
    void parseExtendDefaultPrecision();
    void parseLineNumbers();
    void clearParsedSrc();

    GLenum      m_type;
    char*       m_originalSrc;
    std::string m_src;
    std::string m_parsedSrc;
    GLchar*     m_parsedLines;
    GLchar*     m_infoLog;
    bool        m_deleteStatus;
    GLuint      m_program;
    bool        m_valid;
};
#endif
