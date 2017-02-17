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
#include "android/base/containers/Lookup.h"

#include <GLES3/gl31.h>
#include <GLcommon/objectNameManager.h>
#include "ProgramData.h"

#include <unordered_set>

#include <string.h>

ProgramData::ProgramData() :  ObjectData(PROGRAM_DATA),
                              AttachedVertexShader(0),
                              AttachedFragmentShader(0),
                              AttachedComputeShader(0),
                              LinkStatus(GL_FALSE),
                              IsInUse(false),
                              DeleteStatus(false) {}

ProgramData::ProgramData(android::base::Stream* stream) :
    ObjectData(stream) {
    size_t attribLocNum = stream->getBe32();
    for (size_t attribLoc = 0; attribLoc < attribLocNum; attribLoc ++) {
        std::string attrib = stream->getString();
        GLuint loc = stream->getBe32();
        boundAttribLocs.emplace(std::move(attrib), loc);
    }

    AttachedVertexShader = stream->getBe32();
    AttachedFragmentShader = stream->getBe32();
    AttachedComputeShader = stream->getBe32();
    validationInfoLog = stream->getString();

    std::string infoLogRead = stream->getString();
    size_t infoSize = infoLogRead.length();
    if (infoSize) {
        GLchar* info = new GLchar[infoSize + 1];
        memcpy(info, infoLogRead.c_str(), infoSize + 1);
        infoLog.reset(info);
    }
    LinkStatus = stream->getBe32();
    IsInUse = stream->getByte();
    DeleteStatus = stream->getByte();
}

void ProgramData::onSave(android::base::Stream* stream) const {
    // The first byte is used to distinguish between program and shader object.
    // It will be loaded outside of this class
    stream->putByte(LOAD_PROGRAM);
    ObjectData::onSave(stream);
    stream->putBe32(boundAttribLocs.size());
    for (const auto& attribLocs : boundAttribLocs) {
        stream->putString(attribLocs.first);
        stream->putBe32(attribLocs.second);
    }

    stream->putBe32(AttachedVertexShader);
    stream->putBe32(AttachedFragmentShader);
    stream->putBe32(AttachedComputeShader);
    stream->putString(validationInfoLog);
    stream->putString(infoLog.get());
    stream->putBe32(LinkStatus);
    stream->putByte(IsInUse);
    stream->putByte(DeleteStatus);
}

void ProgramData::setErrInfoLog() {
    size_t bytes = validationInfoLog.length() + 1;
    infoLog.reset(new GLchar[bytes]);
    memcpy((char*)infoLog.get(), &validationInfoLog[0], bytes);
}

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

void ProgramData::bindAttribLocation(const std::string& var, GLuint loc) {
    boundAttribLocs[var] = loc;
}

// Link-time validation
void ProgramData::appendValidationErrMsg(std::ostringstream& ss) {
    validationInfoLog += "Error: " + ss.str() + "\n";
}
static bool sCheckUndecl(ProgramData* pData,
                         const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
                         const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo);
static bool sCheckLimits(ProgramData* pData,
                         const ShBuiltInResources& resources,
                         const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
                         const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo);
static bool sCheckVariables(ProgramData* pData,
                            const ANGLEShaderParser::ShaderLinkInfo& a,
                            const ANGLEShaderParser::ShaderLinkInfo& b);
bool ProgramData::validateLink(ShaderParser* frag, ShaderParser* vert) {
    const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo =
        frag->getShaderLinkInfo();
    const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo =
        vert->getShaderLinkInfo();

    bool res = true;

    res = res && sCheckUndecl(this, fragLinkInfo, vertLinkInfo);
    res = res && sCheckLimits(this, ANGLEShaderParser::kResources,
                              fragLinkInfo, vertLinkInfo);
    res = res && sCheckVariables(this, fragLinkInfo, vertLinkInfo);

    return res;
}

void ProgramData::setLinkStatus(GLint status) {
    LinkStatus = status;
}

GLint ProgramData::getLinkStatus() const {
    return LinkStatus;
}

static const char kDifferentPrecisionErr[] =
    "specified with different precision in different shaders.";
static const char kDifferentTypeErr[] =
    "specified with different type in different shaders.";
static const char kDifferentLayoutQualifierErr[] =
    "specified with different layout qualifiers in different shaders.";
static const char kExceededMaxVertexAttribs[] =
    "exceeded max vertex attribs.";
static const char kUsedUndeclaredErr[] =
    "used, but not declared.";
static const char kUniformQualifier[] = "uniform";
static const char kVaryingQualifier[] = "varying";
static const char kUnknownQualifier[] = "[unknown qualifier]";
enum ValidationQualifier {
    UNIFORM,
    VARYING,
};

static const char* sQualifierString(ValidationQualifier q) {
    switch (q) {
    case ValidationQualifier::UNIFORM:
        return kUniformQualifier;
    case ValidationQualifier::VARYING:
        return kVaryingQualifier;
    }
    return kUnknownQualifier;
}

static bool sVarCheck(ProgramData* pData,
                      ValidationQualifier qualifier,
                      const sh::ShaderVariable& a,
                      const sh::ShaderVariable& b) {
    bool res = true;

    if (qualifier == ValidationQualifier::UNIFORM &&
        a.precision != b.precision) {
        std::ostringstream err;
        err << sQualifierString(qualifier) << " " << a.name << " ";
        err << kDifferentPrecisionErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    if (a.isStruct() != b.isStruct() ||
        a.type != b.type) {
        std::ostringstream err;
        err << sQualifierString(qualifier) << " " << a.name << " ";
        err << kDifferentTypeErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    if (a.isStruct()) {
        for (const auto& afield : a.fields) {
            for (const auto& bfield : b.fields) {
                if (afield.name != bfield.name) continue;
                res = res && sVarCheck(pData, qualifier, afield, bfield);
            }
        }
    }

    return res;
}

static bool sInterfaceBlockCheck(ProgramData* pData,
                                 const sh::InterfaceBlock& a,
                                 const sh::InterfaceBlock& b) {
    bool res = true;

    if (a.layout != b.layout ||
        a.isRowMajorLayout != b.isRowMajorLayout) {
        std::ostringstream err;
        err << "interface block " << a.name << " ";
        err << kDifferentLayoutQualifierErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    if (a.fields.size() != b.fields.size()) {
        std::ostringstream err;
        err << "interface block " << a.name << " ";
        err << kDifferentTypeErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    for (const auto& afield : a.fields) {
        for (const auto& bfield : b.fields) {
            if (afield.name != bfield.name) continue;
            res = res && sVarCheck(pData, ValidationQualifier::VARYING,
                                   afield, bfield);
            if (afield.isRowMajorLayout != bfield.isRowMajorLayout) {
                std::ostringstream err;
                err << "interface block field ";
                err << a.name << "." << afield.name << " ";
                err << kDifferentLayoutQualifierErr;
                pData->appendValidationErrMsg(err);
                res = false;
            }
        }
    }
    return res;
}

static bool sCheckUndecl(
        ProgramData* pData,
        const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
        const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo) {
    bool res = true;
    for (const auto& felt : fragLinkInfo.varyings) {
        if (felt.isBuiltIn()) continue;

        bool declaredInVertShader = false;
        for (const auto& velt : vertLinkInfo.varyings) {
            if (velt.name == felt.name) {
                declaredInVertShader = true;
                break;
            }
        }

        if (!declaredInVertShader && felt.staticUse) {
            std::ostringstream err;
            err << "varying " << felt.name << " ";
            err << kUsedUndeclaredErr;
            pData->appendValidationErrMsg(err);
            res = false;
        }
    }
    return res;
}

static bool sCheckLimits(
        ProgramData* pData,
        const ShBuiltInResources& resources,
        const ANGLEShaderParser::ShaderLinkInfo& fragShaderLinkInfo,
        const ANGLEShaderParser::ShaderLinkInfo& vertShaderLinkInfo) {
    bool res = true;

    size_t maxAttribs = (size_t)resources.MaxVertexAttribs;

    std::unordered_set<GLuint> explicitlyBound;
    int numImplicitlyBound = 0;
    for (const auto& elt : vertShaderLinkInfo.attributes) {
        if (const auto loc = android::base::find(pData->boundAttribLocs, elt.name)) {
            explicitlyBound.insert(*loc);
        } else {
            numImplicitlyBound++;
        }
    }
    int numExplicitlyBound = explicitlyBound.size();

    if ((int)maxAttribs - numExplicitlyBound - numImplicitlyBound < 0) {
        std::ostringstream err;
        err << kExceededMaxVertexAttribs;
        err << " Wanted (from vertex shader): ";
        err << numExplicitlyBound + numImplicitlyBound << " ";
        err << " Limit: " << maxAttribs << " ";
        pData->appendValidationErrMsg(err);
        res = false;
    }

    return res;
}

static bool sCheckVariables(ProgramData* pData,
                            const ANGLEShaderParser::ShaderLinkInfo& a,
                            const ANGLEShaderParser::ShaderLinkInfo& b) {
    bool res = true;

    for (const auto& aelt : a.uniforms) {
        for (const auto& belt : b.uniforms) {
            if (aelt.name != belt.name) continue;
            res = res && sVarCheck(pData, ValidationQualifier::UNIFORM, aelt, belt);
        }
    }

    for (const auto& aelt : a.varyings) {
        for (const auto& belt : b.varyings) {
            if (aelt.name != belt.name) continue;
            res = res && sVarCheck(pData, ValidationQualifier::VARYING, aelt, belt);
        }
    }

    for (const auto& aelt : a.interfaceBlocks) {
        for (const auto& belt : b.interfaceBlocks) {
            if (aelt.name != belt.name) continue;
            res = res && sInterfaceBlockCheck(pData, aelt, belt);
        }
    }

    return res;
}


