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
#ifndef PROGRAM_DATA_H
#define PROGRAM_DATA_H

#include "ShaderParser.h"

#include "android/base/containers/HybridComponentManager.h"
#include "android/base/StringView.h"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

struct GLUniformDesc {
    GLUniformDesc() = default;
    GLUniformDesc(const char* name, GLint location, GLsizei count, GLboolean transpose,
            GLenum type, GLsizei size, unsigned char* val);
    GLUniformDesc(android::base::Stream* stream);
    GLUniformDesc(GLUniformDesc&&) = default;
    GLUniformDesc& operator=(GLUniformDesc&&) = default;

    GLsizei mCount = 0;
    GLboolean mTranspose = GL_FALSE;
    GLenum mType = (GLenum)0;
    std::vector<unsigned char> mVal;

    std::string mGuestName = {};

    void onSave(android::base::Stream* stream) const;
};

struct AttachedShader {
    GLuint localName = 0;
    ShaderParser* shader = nullptr;
    // linkedSource is only updated when glLinkProgram
    // This is the "real" source the hardware is using for the compiled program.
    std::string linkedSource = {};
    ANGLEShaderParser::ShaderLinkInfo linkInfo = {};
};

class ProgramData : public ObjectData {
public:
    enum ShaderType {
        VERTEX = 0,
        FRAGMENT,
        COMPUTE,
        NUM_SHADER_TYPE
    };
    ProgramData(int glesMaj, int glesMin);
    ProgramData(android::base::Stream* stream);
    virtual void onSave(android::base::Stream* stream, unsigned int globalName) const override;
    virtual void postLoad(const getObjDataPtr_t& getObjDataPtr) override;
    // restore() in ProgramData must be executed after shaders
    virtual void restore(ObjectLocalName localName,
           const getGlobalName_t& getGlobalName) override;

    GLuint getAttachedVertexShader() const;
    GLuint getAttachedFragmentShader() const;
    GLuint getAttachedComputeShader() const;
    GLuint getAttachedShader(GLenum type) const;

    android::base::StringView getTranslatedName(android::base::StringView userVarName) const;
    android::base::StringView getDetranslatedName(android::base::StringView driverName) const;

    bool attachShader(GLuint shader, ShaderParser* shaderData, GLenum type);
    bool isAttached(GLuint shader) const;
    bool detachShader(GLuint shader);
    void bindAttribLocation(const std::string& var, GLuint loc);
    void linkedAttribLocation(const std::string& var, GLuint loc);

    void appendValidationErrMsg(std::ostringstream& ss);
    bool validateLink(ShaderParser* frag, ShaderParser* vert);

    bool getValidateStatus() const { return ValidateStatus; }
    void setValidateStatus(bool status) { ValidateStatus = status; }

    // setLinkStatus resets uniform location virtualization as well
    void setHostLinkStatus(GLint status);
    void setLinkStatus(GLint status);
    bool getLinkStatus() const;

    void setErrInfoLog();

    // make sure this is never called with a 0-length log returned from
    // glGetProgramLog; it can be garbage without a null terminator.
    void setInfoLog(const GLchar *log);
    const GLchar* getInfoLog() const;

    bool isInUse() const { return IsInUse; }
    void setInUse(bool inUse) { IsInUse = inUse; }

    bool getDeleteStatus() const { return DeleteStatus; }
    void setDeleteStatus(bool status) { DeleteStatus = status; }

    // boundAttribLocs stores the attribute locations assigned by
    // glBindAttribLocation.
    // It will take effect after glLinkProgram.
    std::unordered_map<std::string, GLuint> boundAttribLocs;
    virtual GenNameInfo getGenNameInfo() const override;
    void addProgramName(GLuint name) { ProgramName = name; }
    GLuint getProgramName() const { return ProgramName; }

    // Virtualize uniform locations
    // It handles location -1 as well
    void initGuestUniformLocForKey(android::base::StringView key);
    void initGuestUniformLocForKey(android::base::StringView key,
                                   android::base::StringView key2);
    int getGuestUniformLocation(const char* uniName);
    int getHostUniformLocation(int guestLocation);

private:
    // linkedAttribLocs stores the attribute locations the guest might
    // know about. It includes all boundAttribLocs before the previous
    // glLinkProgram and all attribute locations retrieved by glGetAttribLocation
    std::unordered_map<std::string, GLuint> linkedAttribLocs;
    std::unordered_map<GLuint, GLUniformDesc> uniforms;
    AttachedShader attachedShaders[NUM_SHADER_TYPE] = {};
    std::string validationInfoLog;
    std::string infoLog;
    bool ValidateStatus;
    bool LinkStatus;
    bool HostLinkStatus;
    bool IsInUse;
    bool DeleteStatus;
    GLuint  ProgramName;
    std::unordered_map<GLuint, GLuint> mUniformBlockBinding;
    std::vector<std::string> mTransformFeedbacks;
    GLenum mTransformFeedbackBufferMode = 0;

    int mGlesMajorVersion = 2;
    int mGlesMinorVersion = 0;
    std::unordered_map<GLuint, GLUniformDesc> collectUniformInfo() const;
    void getUniformValue(const GLchar *name, GLenum type,
            std::unordered_map<GLuint, GLUniformDesc> &uniformsOnSave) const;

    std::unordered_map<std::string, int> mUniNameToGuestLoc;
    android::base::HybridComponentManager<10000, int, int> mGuestLocToHostLoc;

    int mCurrUniformBaseLoc = 0;
    bool mUseUniformLocationVirtualization = true;
    bool mUseDirectDriverUniformInfo = false;
};
#endif
