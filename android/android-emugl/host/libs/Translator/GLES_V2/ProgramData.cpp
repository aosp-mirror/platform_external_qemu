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

#include "ProgramData.h"
#include "OpenglCodecCommon/glUtils.h"

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"
#include "ANGLEShaderParser.h"
#include "GLcommon/GLutils.h"
#include "GLcommon/GLESmacros.h"
#include "GLcommon/ShareGroup.h"

#include <GLES3/gl31.h>
#include <string.h>
#include <unordered_set>

using android::base::c_str;
using android::base::StringView;

GLUniformDesc::GLUniformDesc(const char* name, GLint location, GLsizei count, GLboolean transpose,
            GLenum type, GLsizei size, unsigned char* val)
        : mCount(count), mTranspose(transpose), mType(type)
        , mVal(val, val + size), mGuestName(name) { }

GLUniformDesc::GLUniformDesc(android::base::Stream* stream) {
    mCount = stream->getBe32();
    mTranspose = stream->getByte();
    mType = stream->getBe32();
    loadBuffer(stream, &mVal);
    mGuestName = stream->getString();
}

void GLUniformDesc::onSave(android::base::Stream* stream) const {
    stream->putBe32(mCount);
    stream->putByte(mTranspose);
    stream->putBe32(mType);
    saveBuffer(stream, mVal);
    stream->putString(mGuestName);
}

static int s_glShaderType2ShaderType(GLenum type) {
    switch (type) {
    case GL_VERTEX_SHADER:
        return ProgramData::VERTEX;
        break;
    case GL_FRAGMENT_SHADER:
        return ProgramData::FRAGMENT;
        break;
    case GL_COMPUTE_SHADER:
        return ProgramData::COMPUTE;
        break;
    default:
        assert(0);
        break;
    }
    return ProgramData::NUM_SHADER_TYPE;
}

ProgramData::ProgramData(int glesMaj, int glesMin)
    : ObjectData(PROGRAM_DATA),
      ValidateStatus(false),
      LinkStatus(false),
      HostLinkStatus(false),
      IsInUse(false),
      DeleteStatus(false),
      mGlesMajorVersion(glesMaj),
      mGlesMinorVersion(glesMin) {}

ProgramData::ProgramData(android::base::Stream* stream) :
    ObjectData(stream) {
    auto loadAttribLocs = [](android::base::Stream* stream) {
                std::string attrib = stream->getString();
                GLuint loc = stream->getBe32();
                return std::make_pair(std::move(attrib), loc);
            };
    loadCollection(stream, &boundAttribLocs, loadAttribLocs);
    loadCollection(stream, &linkedAttribLocs, loadAttribLocs);

    loadCollection(stream, &uniforms, [](android::base::Stream* stream) {
       GLuint loc = stream->getBe32();
       GLUniformDesc desc(stream);
       return std::make_pair(loc, std::move(desc));
    });
    loadCollection(stream, &mUniformBlockBinding,
            [](android::base::Stream* stream) {
                GLuint block = stream->getBe32();
                GLuint binding = stream->getBe32();
                return std::make_pair(block, binding);
    });
    int transformFeedbackCount = stream->getBe32();
    mTransformFeedbacks.resize(transformFeedbackCount);
    for (auto& feedback : mTransformFeedbacks) {
        feedback = stream->getString();
    }
    mTransformFeedbackBufferMode = stream->getBe32();

    for (auto& s : attachedShaders) {
        s.localName = stream->getBe32();
        s.linkedSource = stream->getString();
    }
    validationInfoLog = stream->getString();
    infoLog = stream->getString();

    stream->getBe16(); /* padding to maintain snapshot file compatibility */
    ValidateStatus = stream->getByte();
    LinkStatus = stream->getByte();
    IsInUse = stream->getByte();
    DeleteStatus = stream->getByte();

    mGlesMajorVersion = stream->getByte();
    mGlesMinorVersion = stream->getByte();
    loadCollection(stream, &mUniNameToGuestLoc,
            [](android::base::Stream* stream) {
        std::string name = stream->getString();
        int loc = stream->getBe32();
        return std::make_pair(name, loc);
    });
}

void ProgramData::getUniformValue(const GLchar *name, GLenum type,
        std::unordered_map<GLuint, GLUniformDesc> &uniformsOnSave) const {
    alignas(double) unsigned char val[256];     //Large enought to hold MAT4x4
    GLDispatch& dispatcher = GLEScontext::dispatcher();

    GLint location = dispatcher.glGetUniformLocation(ProgramName, name);
    if (location < 0) {
        return;
    }
    switch(type) {
    case GL_FLOAT:
    case GL_FLOAT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_FLOAT_VEC4:
    case GL_FLOAT_MAT2:
    case GL_FLOAT_MAT3:
    case GL_FLOAT_MAT4:
    case GL_FLOAT_MAT2x3:
    case GL_FLOAT_MAT2x4:
    case GL_FLOAT_MAT3x2:
    case GL_FLOAT_MAT3x4:
    case GL_FLOAT_MAT4x2:
    case GL_FLOAT_MAT4x3:
        dispatcher.glGetUniformfv(ProgramName, location, (GLfloat *)val);
        break;
    case GL_INT:
    case GL_INT_VEC2:
    case GL_INT_VEC3:
    case GL_INT_VEC4:
    case GL_BOOL:
    case GL_BOOL_VEC2:
    case GL_BOOL_VEC3:
    case GL_BOOL_VEC4:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        dispatcher.glGetUniformiv(ProgramName, location, (GLint *)val);
        break;
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_VEC2:
    case GL_UNSIGNED_INT_VEC3:
    case GL_UNSIGNED_INT_VEC4:
        dispatcher.glGetUniformuiv(ProgramName, location, (GLuint *)val);
        break;
    default:
        fprintf(stderr, "ProgramData::gtUniformValue: warning: "
                "unsupported uniform type 0x%x\n", type);
        return;
    }
    GLUniformDesc uniformDesc(name, location, 1, 0, /*transpose*/
        type, glSizeof(type), val);

    if (!isGles2Gles()) {
        uniformDesc.mGuestName = getDetranslatedName(uniformDesc.mGuestName);
    }

    uniformsOnSave[location] = std::move(uniformDesc);
}

static std::string getBaseName(const std::string& name) {
    std::string baseName;
    int length = name.length();
    if (length < 3) return name;
    if (name.compare(length - 3, 1, "[") == 0) {
        baseName = name.substr(0, length - 3);
    } else {
        baseName = name;
    }
    return baseName;
}

// Query uniform variables from driver
std::unordered_map<GLuint, GLUniformDesc> ProgramData::collectUniformInfo() const {
    GLint uniform_count = 0;
    GLint nameLength = 0;
    std::unordered_map<GLuint, GLUniformDesc> uniformsOnSave;
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glGetProgramiv(ProgramName, GL_ACTIVE_UNIFORM_MAX_LENGTH, &nameLength);
    if (nameLength == 0) {
        // No active uniform variables exist.
        return uniformsOnSave;
    }
    dispatcher.glGetProgramiv(ProgramName, GL_ACTIVE_UNIFORMS, &uniform_count);
    std::vector<char> name(nameLength, 0);
    for (int i = 0; i < uniform_count; i++) {
        GLint size;
        GLenum type;
        GLsizei length;
        dispatcher.glGetActiveUniform(ProgramName, i, nameLength, &length,
                &size, &type, name.data());
        if (size > 1) {
            // Uniform array, drivers may return 'arrayName' or 'arrayName[0]'
            // as the name of the array.
            // Need to append '[arrayIndex]' after 'arrayName' to query the
            // value for each array member.
            std::string baseName = getBaseName(std::string(name.data()));
            for (int arrayIndex = 0; arrayIndex < size; arrayIndex++) {
                std::ostringstream oss;
                oss << baseName << '[' << arrayIndex << ']';
                std::string toSaveName = oss.str();
                getUniformValue(toSaveName.c_str(), type, uniformsOnSave);
            }
        }
        else {
            getUniformValue(name.data(), type, uniformsOnSave);
        }
    }
    return uniformsOnSave;
}

static std::unordered_map<GLuint, GLuint> collectUniformBlockInfo(GLuint pname) {
    GLint uniformBlockCount = 0;
    std::unordered_map<GLuint, GLuint> uniformBlocks;
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glGetProgramiv(pname, GL_ACTIVE_UNIFORM_BLOCKS,
            &uniformBlockCount);
    for (int i = 0; i < uniformBlockCount; i++) {
        GLint binding = 0;
        dispatcher.glGetActiveUniformBlockiv(pname, i, GL_UNIFORM_BLOCK_BINDING,
                &binding);
        uniformBlocks.emplace(i, binding);
    }
    return uniformBlocks;
}

static std::vector<std::string> collectTransformFeedbackInfo(GLuint pname) {
    GLint transformFeedbackCount = 0;
    GLint transformFeedbakMaxLength = 0;
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glGetProgramiv(pname, GL_TRANSFORM_FEEDBACK_VARYINGS,
            &transformFeedbackCount);
    dispatcher.glGetProgramiv(pname, GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH,
            &transformFeedbakMaxLength);

    std::vector<std::string> transformFeedbacks(transformFeedbackCount);
    std::unique_ptr<char[]> nameBuffer(new char [transformFeedbakMaxLength]);

    for (int i = 0; i < transformFeedbackCount; i++) {
        GLsizei size;
        GLenum type;

        dispatcher.glGetTransformFeedbackVarying(pname, i,
                transformFeedbakMaxLength, nullptr, &size, &type,
                nameBuffer.get());
        transformFeedbacks[i] = nameBuffer.get();
    }
    return transformFeedbacks;
}

void ProgramData::onSave(android::base::Stream* stream, unsigned int globalName) const {
    // The first byte is used to distinguish between program and shader object.
    // It will be loaded outside of this class
    stream->putByte(LOAD_PROGRAM);
    ObjectData::onSave(stream, globalName);
    auto saveAttribLocs = [](android::base::Stream* stream,
            const std::pair<std::string, GLuint>& attribLoc) {
                stream->putString(attribLoc.first);
                stream->putBe32(attribLoc.second);
            };
    saveCollection(stream, boundAttribLocs, saveAttribLocs);
    saveCollection(stream, linkedAttribLocs, saveAttribLocs);

    auto saveUniform = [](android::base::Stream* stream,
                const std::pair<const GLuint, GLUniformDesc>& uniform) {
            stream->putBe32(uniform.first);
            uniform.second.onSave(stream);
        };
    auto saveUniformBlock = [](android::base::Stream* stream,
                const std::pair<const GLuint, GLuint>& uniformBlock) {
            stream->putBe32(uniformBlock.first);
            stream->putBe32(uniformBlock.second);
        };
    auto saveTransformFeedbacks = [](android::base::Stream* stream,
                const std::vector<std::string>& transformFeedbacks) {
            stream->putBe32((int)transformFeedbacks.size());
            for (const auto& feedback : transformFeedbacks) {
                stream->putString(feedback);
            }
        };
    if (needRestore()) {
        saveCollection(stream, uniforms, saveUniform);
        saveCollection(stream, mUniformBlockBinding, saveUniformBlock);
        saveTransformFeedbacks(stream, mTransformFeedbacks);
        stream->putBe32(mTransformFeedbackBufferMode);
    } else {
        std::unordered_map<GLuint, GLUniformDesc> uniformsOnSave =
                collectUniformInfo();
        std::unordered_map<GLuint, GLuint> uniformBlocks;
        std::vector<std::string> transformFeedbacks;
        if (mGlesMajorVersion >= 3) {
            uniformBlocks = collectUniformBlockInfo(ProgramName);
            transformFeedbacks = collectTransformFeedbackInfo(ProgramName);
            GLEScontext::dispatcher().glGetProgramiv(ProgramName,
                    GL_TRANSFORM_FEEDBACK_BUFFER_MODE,
                    (GLint*)&mTransformFeedbackBufferMode);
        }

        saveCollection(stream, uniformsOnSave, saveUniform);
        saveCollection(stream, uniformBlocks, saveUniformBlock);
        saveTransformFeedbacks(stream, transformFeedbacks);
        stream->putBe32(mTransformFeedbackBufferMode);
    }

    for (const auto& s : attachedShaders) {
        stream->putBe32(s.localName);
        stream->putString(s.linkedSource);
        // s.linkedInfo will be regenerated on restore
        // This is for compatibility over different rendering backends
    }
    stream->putString(validationInfoLog);
    stream->putString(infoLog);

    stream->putBe16(0 /* padding to maintain snapshot file compatibility */);
    stream->putByte(ValidateStatus);
    stream->putByte(LinkStatus);

    stream->putByte(IsInUse);
    stream->putByte(DeleteStatus);

    stream->putByte(mGlesMajorVersion);
    stream->putByte(mGlesMinorVersion);
    saveCollection(stream, mUniNameToGuestLoc, [](android::base::Stream* stream,
                const std::pair<std::string, int>& uniNameLoc) {
        stream->putString(uniNameLoc.first);
        stream->putBe32(uniNameLoc.second);
    });
}

void ProgramData::postLoad(const getObjDataPtr_t& getObjDataPtr) {
    for (auto& s : attachedShaders) {
        if (s.localName) {
            s.shader = (ShaderParser*)getObjDataPtr(
                    NamedObjectType::SHADER_OR_PROGRAM, s.localName).get();
        }
    }
}

void ProgramData::restore(ObjectLocalName localName,
           const getGlobalName_t& getGlobalName) {
    ObjectData::restore(localName, getGlobalName);
    int globalName = getGlobalName(NamedObjectType::SHADER_OR_PROGRAM,
            localName);
    assert(globalName);
    ProgramName = globalName;
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    mGuestLocToHostLoc.add(-1, -1);
    bool shoudLoadLinked = LinkStatus;
#if defined(TOLERATE_PROGRAM_LINK_ERROR) && TOLERATE_PROGRAM_LINK_ERROR == 1
    shoudLoadLinked = 1;
#endif
    if (shoudLoadLinked) {
        // Really, each program name corresponds to 2 programs:
        // the one that is already linked, and the one that is not yet linked.
        // We need to restore both.
        GLint tmpShaders[NUM_SHADER_TYPE];
        for (int i = 0; i < NUM_SHADER_TYPE; i++) {
            AttachedShader& s = attachedShaders[i];
            if (s.linkedSource.empty()) {
                tmpShaders[i] = 0;
                continue;
            }
            GLenum type = 0;
            switch (i) {
            case VERTEX:
                type = GL_VERTEX_SHADER;
                break;
            case FRAGMENT:
                type = GL_FRAGMENT_SHADER;
                break;
            case COMPUTE:
                type = GL_COMPUTE_SHADER;
                break;
            default:
                assert(0);
            }
            tmpShaders[i] = dispatcher.glCreateShader(type);
            const GLchar* src = (const GLchar *)s.linkedSource.c_str();
            std::string parsedSrc;
            if (!isGles2Gles()) {
                std::string infoLog;
                ANGLEShaderParser::translate(
                            isCoreProfile(),
                            src,
                            type,
                            &infoLog,
                            &parsedSrc,
                            &s.linkInfo);
                src = parsedSrc.c_str();
            }
            dispatcher.glShaderSource(tmpShaders[i], 1, &src, NULL);
            dispatcher.glCompileShader(tmpShaders[i]);
            dispatcher.glAttachShader(globalName, tmpShaders[i]);
        }
        for (const auto& attribLocs : linkedAttribLocs) {
            // Prefix "gl_" is reserved, we should skip those.
            // https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glBindAttribLocation.xhtml
            if  (strncmp(attribLocs.first.c_str(), "gl_", 3) == 0) {
                continue;
            }
            dispatcher.glBindAttribLocation(globalName, attribLocs.second,
                    attribLocs.first.c_str());
        }
        if (mGlesMajorVersion >= 3) {
            std::vector<const char*> varyings;
            varyings.resize(mTransformFeedbacks.size());
            for (size_t i = 0; i < mTransformFeedbacks.size(); i++) {
                varyings[i] = mTransformFeedbacks[i].c_str();
            }
            dispatcher.glTransformFeedbackVaryings(
                    globalName, mTransformFeedbacks.size(), varyings.data(),
                    mTransformFeedbackBufferMode);
            mTransformFeedbacks.clear();
        }
        dispatcher.glLinkProgram(globalName);
        dispatcher.glUseProgram(globalName);
#ifdef DEBUG
        for (const auto& attribLocs : linkedAttribLocs) {
            assert(dispatcher.glGetAttribLocation(globalName,
                attribLocs.first.c_str()) == attribLocs.second);
        }
#endif // DEBUG
        for (const auto& uniform : mUniNameToGuestLoc) {
            GLint hostLoc = dispatcher.glGetUniformLocation(
                    globalName, c_str(getTranslatedName(uniform.first)));
            if (hostLoc != -1) {
                mGuestLocToHostLoc.add(uniform.second, hostLoc);
            }
        }
        for (const auto& uniformEntry : uniforms) {
            const auto& uniform = uniformEntry.second;
            GLint location = dispatcher.glGetUniformLocation(
                    globalName, c_str(getTranslatedName(uniform.mGuestName)));
            if (location == -1) {
                // Location changed after loading from a snapshot.
                // likely loading from different GPU backend (and they
                // optimize out different stuff)
                continue;
            }

            switch (uniform.mType) {
                case GL_FLOAT:
                    dispatcher.glUniform1fv(location, uniform.mCount,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_VEC2:
                    dispatcher.glUniform2fv(location, uniform.mCount,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_VEC3:
                    dispatcher.glUniform3fv(location, uniform.mCount,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_VEC4:
                    dispatcher.glUniform4fv(location, uniform.mCount,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_BOOL:
                case GL_INT:
                case GL_SAMPLER_2D:
                case GL_SAMPLER_3D:
                case GL_SAMPLER_CUBE:
                case GL_SAMPLER_2D_SHADOW:
                case GL_SAMPLER_2D_ARRAY:
                case GL_SAMPLER_2D_ARRAY_SHADOW:
                case GL_SAMPLER_CUBE_SHADOW:
                case GL_INT_SAMPLER_2D:
                case GL_INT_SAMPLER_3D:
                case GL_INT_SAMPLER_CUBE:
                case GL_INT_SAMPLER_2D_ARRAY:
                case GL_UNSIGNED_INT_SAMPLER_2D:
                case GL_UNSIGNED_INT_SAMPLER_3D:
                case GL_UNSIGNED_INT_SAMPLER_CUBE:
                case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
                    dispatcher.glUniform1iv(location, uniform.mCount,
                            (const GLint*)uniform.mVal.data());
                    break;
                case GL_BOOL_VEC2:
                case GL_INT_VEC2:
                    dispatcher.glUniform2iv(location, uniform.mCount,
                            (const GLint*)uniform.mVal.data());
                    break;
                case GL_BOOL_VEC3:
                case GL_INT_VEC3:
                    dispatcher.glUniform3iv(location, uniform.mCount,
                            (const GLint*)uniform.mVal.data());
                    break;
                case GL_BOOL_VEC4:
                case GL_INT_VEC4:
                    dispatcher.glUniform4iv(location, uniform.mCount,
                            (const GLint*)uniform.mVal.data());
                    break;
                case GL_UNSIGNED_INT:
                    dispatcher.glUniform1uiv(location, uniform.mCount,
                            (const GLuint*)uniform.mVal.data());
                    break;
                case GL_UNSIGNED_INT_VEC2:
                    dispatcher.glUniform2uiv(location, uniform.mCount,
                            (const GLuint*)uniform.mVal.data());
                    break;
                case GL_UNSIGNED_INT_VEC3:
                    dispatcher.glUniform3uiv(location, uniform.mCount,
                            (const GLuint*)uniform.mVal.data());
                    break;
                case GL_UNSIGNED_INT_VEC4:
                    dispatcher.glUniform4uiv(location, uniform.mCount,
                            (const GLuint*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT2:
                    dispatcher.glUniformMatrix2fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT3:
                    dispatcher.glUniformMatrix3fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT4:
                    dispatcher.glUniformMatrix4fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT2x3:
                    dispatcher.glUniformMatrix2x3fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT2x4:
                    dispatcher.glUniformMatrix2x4fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT3x2:
                    dispatcher.glUniformMatrix3x2fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT3x4:
                    dispatcher.glUniformMatrix3x4fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT4x2:
                    dispatcher.glUniformMatrix4x2fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT4x3:
                    dispatcher.glUniformMatrix4x3fv(location,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                default:
                    fprintf(stderr, "ProgramData::restore: warning: "
                            "unsupported uniform type 0x%x\n", uniform.mType);
            }
        }
        for (const auto& uniformBlock : mUniformBlockBinding) {
            dispatcher.glUniformBlockBinding(globalName, uniformBlock.first,
                    uniformBlock.second);
        }
        for (auto s : tmpShaders) {
            if (s != 0) {
                dispatcher.glDetachShader(globalName, s);
                dispatcher.glDeleteShader(s);
            }
        }
    }
    uniforms.clear();
    mUniformBlockBinding.clear();
    // We are done with the "linked" program, now we handle the one
    // that is yet to compile
    for (const auto& s : attachedShaders) {
        if (s.localName) {
            int shaderGlobalName = getGlobalName(
                    NamedObjectType::SHADER_OR_PROGRAM, s.localName);
            assert(shaderGlobalName);
            dispatcher.glAttachShader(globalName, shaderGlobalName);
        }
    }
    for (const auto& attribLocs : boundAttribLocs) {
        dispatcher.glBindAttribLocation(globalName, attribLocs.second,
                attribLocs.first.c_str());
    }
}

GenNameInfo ProgramData::getGenNameInfo() const {
    return GenNameInfo(ShaderProgramType::PROGRAM);
}

void ProgramData::setErrInfoLog() {
    infoLog.clear();
    infoLog = std::string(validationInfoLog);
}

void ProgramData::setInfoLog(const GLchar* log) {
    infoLog = std::string(log);
}

const GLchar* ProgramData::getInfoLog() const {
    return infoLog.c_str();
}

GLuint ProgramData::getAttachedVertexShader() const {
    return attachedShaders[VERTEX].localName;
}

GLuint ProgramData::getAttachedFragmentShader() const {
    return attachedShaders[FRAGMENT].localName;
}

GLuint ProgramData::getAttachedComputeShader() const {
    return attachedShaders[COMPUTE].localName;
}

GLuint ProgramData::getAttachedShader(GLenum type) const {
    return attachedShaders[s_glShaderType2ShaderType(type)].localName;
}

StringView
ProgramData::getTranslatedName(StringView userVarName) const {
    if (isGles2Gles()) {
        return userVarName;
    }
    // TODO: translate uniform array names
    for (int i = 0; i < NUM_SHADER_TYPE; i++) {
        if (const auto name = android::base::find(
                attachedShaders[i].linkInfo.nameMap, userVarName)) {
            return *name;
        }
    }
    return userVarName;
}

StringView
ProgramData::getDetranslatedName(StringView driverName) const {
    if (isGles2Gles()) {
        return driverName;
    }
    // TODO: detranslate uniform array names
    for (int i = 0; i < NUM_SHADER_TYPE; i++) {
        if (const auto name = android::base::find(
                attachedShaders[i].linkInfo.nameMapReverse, driverName)) {
            return *name;
        }
    }
    return driverName;
}

bool ProgramData::attachShader(GLuint shader, ShaderParser* shaderData,
        GLenum type) {
    AttachedShader& s = attachedShaders[s_glShaderType2ShaderType(type)];
    if (s.localName == 0) {
        s.localName = shader;
        s.shader = shaderData;
        return true;
    }
    return false;
}

bool ProgramData::isAttached(GLuint shader) const {
    for (const auto& s : attachedShaders) {
        if (s.localName == shader) return true;
    }
    return false;
}

bool ProgramData::detachShader(GLuint shader) {
    for (auto& s : attachedShaders) {
        if (s.localName == shader) {
            s.localName = 0;
            s.shader = nullptr;
            return true;
        }
    }
    return false;
}

void ProgramData::bindAttribLocation(const std::string& var, GLuint loc) {
    boundAttribLocs[var] = loc;
}

void ProgramData::linkedAttribLocation(const std::string& var, GLuint loc) {
    linkedAttribLocs[var] = loc;
}

// Link-time validation
void ProgramData::appendValidationErrMsg(std::ostringstream& ss) {
    validationInfoLog += "Error: " + ss.str() + "\n";
}
static bool sCheckUndecl(ProgramData* pData,
                         const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
                         const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo);
static bool sCheckLimits(ProgramData* pData,
                         const ST_BuiltInResources& resources,
                         const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
                         const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo);
static bool sCheckVariables(ProgramData* pData,
                            const ANGLEShaderParser::ShaderLinkInfo& a,
                            const ANGLEShaderParser::ShaderLinkInfo& b);
static void sInitializeUniformLocs(ProgramData* pData,
                                   const std::vector<ST_ShaderVariable>& uniforms);

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

void ProgramData::setHostLinkStatus(GLint status) {
    HostLinkStatus = (status == GL_FALSE) ? false : true;
}

void ProgramData::setLinkStatus(GLint status) {
    LinkStatus = (status == GL_FALSE) ? false : true;
    mUniNameToGuestLoc.clear();
    mGuestLocToHostLoc.clear();
    mGuestLocToHostLoc.add(-1, -1);
#if defined(TOLERATE_PROGRAM_LINK_ERROR) && TOLERATE_PROGRAM_LINK_ERROR == 1
    status = 1;
#endif
    if (status && HostLinkStatus) {
        std::vector<ST_ShaderVariable> allUniforms;
        bool is310 = false;
        for (auto& s : attachedShaders) {
            if (s.localName) {
                assert(s.shader);
                s.linkedSource = s.shader->getOriginalSrc();
                s.linkInfo = s.shader->getShaderLinkInfo();
                is310 = is310 || (s.linkInfo.esslVersion == 310);
                for (const auto& var: s.linkInfo.uniforms) {
                    allUniforms.push_back(var);
                }
            }
        }

        if (is310 || isGles2Gles()) {
            mUseDirectDriverUniformInfo = true;
        } else {
            sInitializeUniformLocs(this, allUniforms);
        }
        for (const auto &attribLoc : boundAttribLocs) {
            // overwrite
            linkedAttribLocs[attribLoc.first] = attribLoc.second;
        }
    } else {
        for (auto& s : attachedShaders) {
            s.linkedSource.clear();
        }
    }
}

bool ProgramData::getLinkStatus() const {
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
                      const ST_ShaderVariable& a,
                      const ST_ShaderVariable& b) {
    bool res = true;

    if (qualifier == ValidationQualifier::UNIFORM &&
        a.precision != b.precision) {
        std::ostringstream err;
        err << sQualifierString(qualifier) << " " << a.name << " ";
        err << kDifferentPrecisionErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    bool aIsStruct = a.fieldsCount > 0;
    bool bIsStruct = b.fieldsCount > 0;

    if (aIsStruct != bIsStruct ||
        a.type != b.type) {
        std::ostringstream err;
        err << sQualifierString(qualifier) << " " << a.name << " ";
        err << kDifferentTypeErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    if (aIsStruct) {
        for (unsigned int i = 0; i < a.fieldsCount; ++i) {
            for (unsigned int j = 0; j < b.fieldsCount; ++j) {
                if (strcmp(a.pFields[i].name, b.pFields[j].name)) continue;
                res = res && sVarCheck(pData, qualifier, a.pFields[i], b.pFields[j]);
            }
        }
    }

    return res;
}

static bool sInterfaceBlockCheck(ProgramData* pData,
                                 const ST_InterfaceBlock& a,
                                 const ST_InterfaceBlock& b) {
    bool res = true;

    if (a.layout != b.layout ||
        a.isRowMajorLayout != b.isRowMajorLayout) {
        std::ostringstream err;
        err << "interface block " << a.name << " ";
        err << kDifferentLayoutQualifierErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    if (a.fieldsCount != b.fieldsCount) {
        std::ostringstream err;
        err << "interface block " << a.name << " ";
        err << kDifferentTypeErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    for (unsigned int i = 0; i < a.fieldsCount; ++i) {
        for (unsigned int j = 0; j < b.fieldsCount; ++j) {
            const auto afield = a.pFields[i];
            const auto bfield = b.pFields[j];

            if (strcmp(afield.name, bfield.name)) continue;
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

static bool sIsBuiltInShaderVariable(const ST_ShaderVariable& var) {
    if (!var.name || strlen(var.name) < 4) return false;

    const char* name = var.name;
    return (name[0] == 'g' && name[1] == 'l' && name[2] == '_');
}

static bool sCheckUndecl(
        ProgramData* pData,
        const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
        const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo) {
    bool res = true;
    for (const auto& felt : fragLinkInfo.varyings) {
        if (sIsBuiltInShaderVariable(felt)) continue;

        bool declaredInVertShader = false;
        for (const auto& velt : vertLinkInfo.varyings) {
            if (!strcmp(velt.name, felt.name)) {
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
        const ST_BuiltInResources& resources,
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
            if (strcmp(aelt.name, belt.name)) continue;
            res = res && sVarCheck(pData, ValidationQualifier::UNIFORM, aelt, belt);
        }
    }

    for (const auto& aelt : a.varyings) {
        for (const auto& belt : b.varyings) {
            if (strcmp(aelt.name, belt.name)) continue;
            res = res && sVarCheck(pData, ValidationQualifier::VARYING, aelt, belt);
        }
    }

    for (const auto& aelt : a.interfaceBlocks) {
        for (const auto& belt : b.interfaceBlocks) {
            if (strcmp(aelt.name, belt.name)) continue;
            res = res && sInterfaceBlockCheck(pData, aelt, belt);
        }
    }

    return res;
}

static void sRecursiveLocInitalize(ProgramData* pData, const std::string& keyBase, const ST_ShaderVariable& var) {
    // fprintf(stderr, "%s: call. name: %s\n", __func__, var.name);
    bool isArr = var.arraySizeCount > 0;
    int baseSize = isArr ? var.pArraySizes[0] : 1;
    bool isStruct = var.fieldsCount > 0;

    if (isStruct) {
        // fprintf(stderr, "%s: is struct\n", __func__);
        if (isArr) {
            // fprintf(stderr, "%s: is arr\n", __func__);
            for (int k = 0; k < var.pArraySizes[0]; k++) {
                for (uint32_t i = 0; i < var.fieldsCount; ++i) {
                    std::vector<char> keyBuf(keyBase.length() + strlen(var.pFields[i].name) + 20, 0);
                    snprintf(keyBuf.data(), keyBuf.size(), "%s[%d].%s", keyBase.c_str(), k, var.pFields[i].name);
                    sRecursiveLocInitalize(pData, std::string(keyBuf.data()), var.pFields[i]);
                }
            }
        } else {
            // fprintf(stderr, "%s: is plain struct\n", __func__);
            for (uint32_t i = 0; i < var.fieldsCount; ++i) {
                std::vector<char> keyBuf(keyBase.length() + strlen(var.pFields[i].name) + 20, 0);
                snprintf(keyBuf.data(), keyBuf.size(), "%s.%s", keyBase.c_str(), var.pFields[i].name);
                // fprintf(stderr, "%s: keyBuf: %s\n", __func__, keyBuf.data());
                sRecursiveLocInitalize(pData, std::string(keyBuf.data()), var.pFields[i]);
            }
        }
    } else {
        // fprintf(stderr, "%s: is not struct\n", __func__);
        for (int k = 0; k < baseSize; k++) {
            if (k == 0) {
                std::vector<char> keyBuf(keyBase.length() + 20, 0);
                std::vector<char> keyBuf2(keyBase.length() + 20, 0);
                snprintf(keyBuf.data(), keyBuf.size(), "%s", keyBase.c_str());
                snprintf(keyBuf2.data(), keyBuf.size(), "%s[%d]", keyBase.c_str(), k);
                // fprintf(stderr, "%s: initGuestUniformLocForKey. keyBuf2 %s\n", __func__, keyBuf2.data());
                pData->initGuestUniformLocForKey(keyBuf.data(), keyBuf2.data());
            } else {
                std::vector<char> keyBuf(keyBase.length() + 20, 0);
                snprintf(keyBuf.data(), keyBuf.size(), "%s[%d]", keyBase.c_str(), k);
                // fprintf(stderr, "%s: initGuestUniformLocForKey. keyBu2 %s\n", __func__, keyBuf.data());
                pData->initGuestUniformLocForKey(keyBuf.data());
            }
        }
    }
}

static void sInitializeUniformLocs(ProgramData* pData,
                                   const std::vector<ST_ShaderVariable>& uniforms) {
    // fprintf(stderr, "%s: call\n", __func__);
    // initialize in order of indices
    std::vector<std::string> orderedUniforms;
    GLint uniform_count;
    GLint nameLength;

    GLDispatch& gl = GLEScontext::dispatcher();
    gl.glGetProgramiv(pData->getProgramName(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &nameLength);
    gl.glGetProgramiv(pData->getProgramName(), GL_ACTIVE_UNIFORMS, &uniform_count);

    std::vector<char> name(nameLength, 0);

    for (int i = 0; i < uniform_count; i++) {
        GLint size;
        GLenum type;
        GLsizei length;
        gl.glGetActiveUniform(pData->getProgramName(), i, nameLength, &length,
                &size, &type, name.data());
        // fprintf(stderr, "%s: host uniform: %s\n", __func__, name.data());
        orderedUniforms.push_back(pData->getDetranslatedName(name.data()));
        // fprintf(stderr, "%s: host uniform detranslated: %s\n", __func__, pData->getDetranslatedName(name.data()).str().c_str());
    }

    std::unordered_map<std::string, size_t> linkInfoUniformsByName;

    size_t i = 0;
    for (const auto& var : uniforms) {
        linkInfoUniformsByName[var.name] = i;
        i++;
    }

    for (const auto& str : orderedUniforms) {
        // fprintf(stderr, "%s: do ordered uniforms\n", __func__);
        if (linkInfoUniformsByName.find(str) != linkInfoUniformsByName.end()) {
            sRecursiveLocInitalize(pData, str, uniforms[linkInfoUniformsByName[str]]);
        } else {
            // fprintf(stderr, "%s: found in link info uniforms\n", __func__);
        }
    }

    for (const auto& var : uniforms) {
        sRecursiveLocInitalize(pData, var.name, var);
    }
}

void ProgramData::initGuestUniformLocForKey(StringView key) {
    // fprintf(stderr, "%s: for %s\n", __func__, key.str().c_str());
    if (mUniNameToGuestLoc.find(key) == mUniNameToGuestLoc.end()) {
        mUniNameToGuestLoc[key] = mCurrUniformBaseLoc;
        // Emplace host location beforehand to workaround Unreal bug
        // BUG: 120548998
        GLDispatch& dispatcher = GLEScontext::dispatcher();
        std::string translatedName = getTranslatedName(key);
        // fprintf(stderr, "%s: trname: %s\n", __func__, translatedName.c_str());
        int hostLoc = dispatcher.glGetUniformLocation(ProgramName,
                translatedName.c_str());
        if (hostLoc != -1) {
            mGuestLocToHostLoc.add(mCurrUniformBaseLoc, hostLoc);
        }

        mCurrUniformBaseLoc++;
    }
}

void ProgramData::initGuestUniformLocForKey(StringView key, StringView key2) {
    bool newUniform = false;
    if (mUniNameToGuestLoc.find(key) == mUniNameToGuestLoc.end()) {
        mUniNameToGuestLoc[key] = mCurrUniformBaseLoc;
        newUniform = true;
    }
    if (mUniNameToGuestLoc.find(key2) == mUniNameToGuestLoc.end()) {
        mUniNameToGuestLoc[key2] = mCurrUniformBaseLoc;
        newUniform = true;
    }

    if (newUniform) {
        // Emplace host location beforehand to workaround Unreal bug
        // BUG: 120548998
        GLDispatch& dispatcher = GLEScontext::dispatcher();
        std::string translatedName = getTranslatedName(key);
        int hostLoc = dispatcher.glGetUniformLocation(ProgramName,
                translatedName.c_str());
        if (hostLoc != -1) {
            mGuestLocToHostLoc.add(mCurrUniformBaseLoc, hostLoc);
        }

        mCurrUniformBaseLoc++;
    }
}

int ProgramData::getGuestUniformLocation(const char* uniName) {
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    if (mUseUniformLocationVirtualization) {
        if (mUseDirectDriverUniformInfo) {
            int guestLoc;
            const auto& activeLoc = mUniNameToGuestLoc.find(uniName);
            // If using direct driver uniform info, don't overwrite any
            // previously known guest location.
            if (activeLoc != mUniNameToGuestLoc.end()) {
                guestLoc = activeLoc->second;
            } else {
                guestLoc =
                    dispatcher.glGetUniformLocation(ProgramName, uniName);
                if (guestLoc == -1) {
                    return -1;
                } else {
                    mUniNameToGuestLoc[uniName] = guestLoc;
                    mGuestLocToHostLoc.add(guestLoc, guestLoc);
                }
            }
            return guestLoc;
        } else {
            int guestLoc;

            const auto& activeLoc = mUniNameToGuestLoc.find(uniName);

            if (activeLoc != mUniNameToGuestLoc.end()) {
                guestLoc = activeLoc->second;
            } else {
                guestLoc = -1;
            }

            std::string translatedName = getTranslatedName(uniName);
            int hostLoc = dispatcher.glGetUniformLocation(ProgramName,
                    translatedName.c_str());
            if (hostLoc == -1) {
                return -1;
            }

            mGuestLocToHostLoc.add(guestLoc, hostLoc);
            return guestLoc;
        }
    } else {
        return dispatcher.glGetUniformLocation(
                ProgramName, c_str(getTranslatedName(uniName)));
    }
}

int ProgramData::getHostUniformLocation(int guestLocation) {
    if (mUseUniformLocationVirtualization) {
        if (guestLocation == -1) return -1;

        auto locPtr = mGuestLocToHostLoc.get_const(guestLocation);
        if (!locPtr) return -2;
        return *locPtr;
    } else {
        return guestLocation;
    }
}
