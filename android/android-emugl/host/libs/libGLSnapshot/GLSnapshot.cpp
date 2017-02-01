#include "GLSnapshot.h"

#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 0

#if DEBUG

#define D(fmt,...) do { \
    fprintf(stderr, "%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while(0) \

#else
#define D(...)
#endif

namespace GLSnapshot {

GLSnapshotState::GLSnapshotState(const GLESv2Dispatch* gl) : mGL(gl) {
    D("init snapshot state");

    for (unsigned int i = 0; i < ObjectType::NUM_OBJECT_TYPES; i++) {
        mNextName[i] = 1;
    }
}

void GLSnapshotState::getGlobalStateEnum(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<GLenum>& store = mGlobals[name].enums;
    store.resize(size);
    mGL->glGetIntegerv(name, (GLint*)&store[0]);
}

void GLSnapshotState::getGlobalStateByte(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<unsigned char>& store = mGlobals[name].bytes;
    store.resize(size);
    mGL->glGetBooleanv(name, (GLboolean*)&store[0]);
}

void GLSnapshotState::getGlobalStateInt(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<uint32_t>& store = mGlobals[name].ints;
    store.resize(size);
    mGL->glGetIntegerv(name, (GLint*)&store[0]);
}

void GLSnapshotState::getGlobalStateFloat(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<float>& store = mGlobals[name].floats;
    store.resize(size);
    mGL->glGetFloatv(name, (GLfloat*)&store[0]);
}

void GLSnapshotState::getGlobalStateInt64(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<uint64_t>& store = mGlobals[name].int64s;
    store.resize(size);
    mGL->glGetInteger64v(name, (GLint64*)&store[0]);
}

void GLSnapshotState::getGlobalStateEnable(GLenum name) {
    D("save 0x%x", name);
    mEnables[name] = mGL->glIsEnabled(name) == GL_TRUE;
}

void GLSnapshotState::saveUniform(
         GLint location, const GLsizei count, const GLboolean transpose,
         const GLValueType& type, const GLValue& value) {

    GLProgramState& prog = mPrograms[mCurrentProgram];
    GLUniformDesc res;
    res.count = count;
    res.transpose = transpose;
    res.valtype = type;
    res.val = value;

    prog.uniformDescs[location] = res;
}

void GLSnapshotState::restoreUniform(GLint location, const GLUniformDesc& desc) {
    GLsizei count = desc.count;
    GLboolean transpose = desc.transpose;
    GLValueType type = desc.valtype;
    GLValue value = desc.val;
    
    switch (type) {
    case GLValueType::VALTYPE_1I:
        mGL->glUniform1i(location, value.ints[0]);
        break;
    case GLValueType::VALTYPE_1F:
        mGL->glUniform1f(location, value.floats[0]);
        break;
    case GLValueType::VALTYPE_4F:
        mGL->glUniform4f(location,
                         value.floats[0],
                         value.floats[1],
                         value.floats[2],
                         value.floats[3]);
        break;
    case GLValueType::VALTYPE_4FV:
        mGL->glUniform4fv(location,
                          count,
                          &value.floats[0]);
        break;
    case GLValueType::VALTYPE_M4FV:
        mGL->glUniformMatrix4fv(location,
                                count,
                                transpose,
                                &value.floats[0]);
        break;
        // TODO: rest of the uniform types
    default:
        break;
    }
}

GLuint GLSnapshotState::toPhysName(ObjectType type, GLuint name) {
    return mNames[type][name];
}

GLuint GLSnapshotState::toVirtName(ObjectType type, GLuint name) {
    return mNamesBack[type][name];
}


void GLSnapshotState::save() {
    // Save all global state:
    // Enables/disables
    getGlobalStateEnable(GL_DEPTH_TEST);
    getGlobalStateEnable(GL_SCISSOR_TEST);
    getGlobalStateEnable(GL_BLEND);

    // Clear color, blend func, depth range
    getGlobalStateFloat(GL_COLOR_CLEAR_VALUE, 4);

    getGlobalStateInt(GL_BLEND_SRC_RGB, 1);
    getGlobalStateInt(GL_BLEND_SRC_ALPHA, 1);
    getGlobalStateInt(GL_BLEND_DST_RGB, 1);
    getGlobalStateInt(GL_BLEND_DST_ALPHA, 1);

    // Viewport / scissor
    getGlobalStateInt(GL_VIEWPORT, 4);
    getGlobalStateInt(GL_SCISSOR_BOX, 4);

    // Global program state
    getGlobalStateInt(GL_CURRENT_PROGRAM, 1);

    // Global buffer bindings
    getGlobalStateInt(GL_ARRAY_BUFFER_BINDING, 1);
    getGlobalStateInt(GL_ELEMENT_ARRAY_BUFFER_BINDING, 1);
    // TODO
    // getGlobalStateInt(GL_COPY_READ_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_COPY_WRITE_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_PIXEL_PACK_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_PIXEL_UNPACK_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_UNIFORM_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_SHADER_STORAGE_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_ATOMIC_COUNTER_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_DISPATCH_INDIRECT_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_DRAW_INDIRECT_BUFFER_BINDING, 1);

    // Global texture state
    getGlobalStateInt(GL_ACTIVE_TEXTURE, 1);
    getGlobalStateInt(GL_TEXTURE_BINDING_2D, 1);

    getGlobalStateInt(GL_FRAMEBUFFER_BINDING, 1);

    // TODO: For performance, delay saving uniforms until snapshot save
    // Save program uniform state for all programs
    // char* uniformNameBuf = nullptr;
    // for (const auto& it : mPrograms) {
    //     GLuint program_v = it.first;
    //     GLuint program_p = toPhysName(ObjectType::PROGRAM, program_v);

    //     GLint numActiveUniforms;
    //     GLint activeUniformMaxLen;

    //     glGetProgramiv(program_p, GL_ACTIVE_UNIFORMS, &numActiveUniforms);

    //     if (!numActiveUniforms) continue;

    //     glGetProgramiv(program_p, GL_ACTIVE_UNIFORMS, &activeUniformMaxLen);

    //     uniformNameBuf = new char[activeUniformMaxLen];

    //     for (int i = 0; i < numActiveUniforms; i++) {
    //         GLint uniformCount;
    //         GLenum uniformType;
    //         mGL->glGetActiveUniform(
    //                 program_p, i,
    //                 activeUniformMaxLen,
    //                 NULL,
    //                 &uniformCount,
    //                 &uniformType,
    //                 uniformNameBuf);

    //         // Skip built-in uniform variables
    //         if (!strncmp(uniformNameBuf, "gl_", 3)) continue;

    //         switch (uniformType) {
    //         case GL_FLOAT:
    //         case GL_FLOAT_VEC2:
    //         case GL_FLOAT_VEC3:
    //         case GL_FLOAT_VEC4:
    //         case GL_INT:
    //         case GL_INT_VEC2:
    //         case GL_INT_VEC3:
    //         case GL_INT_VEC4:
    //         case GL_BOOL:
    //         case GL_BOOL_VEC2:
    //         case GL_BOOL_VEC3:
    //         case GL_BOOL_VEC4:
    //         case GL_FLOAT_MAT2:
    //         case GL_FLOAT_MAT3:
    //         case GL_FLOAT_MAT4:
    //         case GL_SAMPLER_2D:
    //         case GL_SAMPLER_CUBE:
    //         default:
    //             break;
    //         }
    //     }
    //     delete [] uniformNameBuf;
    // }
}

void GLSnapshotState::restore() {
    // First do all enables + objects, then do globals, since globals
    // end up binding stuf.

    for (const auto& it : mEnables) {
        if (it.second) {
            mGL->glEnable(it.first);
        } else {
            mGL->glDisable(it.first);
        }
    }

    // Copy current set of names
    GLTypedNameMap currNames(mNames);

    // Shader objects
    for (auto& it: currNames[ObjectType::PROGRAM]) {
        GLuint shaderName = it.first;
        bool isShader = mShaders.find(shaderName) != mShaders.end();

        if (!isShader) continue;

        const GLShaderState& shaderState = mShaders[shaderName];

        GLuint newPhysName = mGL->glCreateShader(shaderState.type);
        refreshName(ObjectType::PROGRAM, shaderName, newPhysName);

        if (shaderState.source.size()) {
            GLint len = shaderState.source.size();
            const char* source = shaderState.source.c_str();
            const char** sources = &source;
            mGL->glShaderSource(newPhysName, 1, sources, &len);
        }
        if (shaderState.compileStatus) {
            mGL->glCompileShader(newPhysName);
        }
    }
    
    // Program objects
    for (auto& it: currNames[ObjectType::PROGRAM]) {
        GLuint programName = it.first;
        bool isProgram = mPrograms.find(programName) != mPrograms.end();

        if (!isProgram) continue;

        GLuint newPhysName = mGL->glCreateProgram();
        refreshName(ObjectType::PROGRAM, programName, newPhysName);

        const GLProgramState& programState = mPrograms[programName];

        for (const auto& jt : programState.linkage) {
            GLuint shaderPhys = toPhysName(ObjectType::PROGRAM, jt.second);
            mGL->glAttachShader(newPhysName, shaderPhys);
        }

        if (programState.linkStatus) {
            mGL->glLinkProgram(newPhysName);
            mGL->glUseProgram(newPhysName);

            for (const auto& jt : programState.boundAttribLocs) {
                mGL->glBindAttribLocation(newPhysName, jt.first, jt.second.c_str());
            }

            for (const auto& jt : programState.uniformDescs) {
                GLint uniformLoc = jt.first;
                const GLUniformDesc& u = jt.second;
                restoreUniform(uniformLoc, u);
            }

            mGL->glUseProgram(0);
        }
    }

    // Buffer objects
    for (auto& it: currNames[ObjectType::BUFFER]) {
        GLuint bufferName = it.first;

        GLuint newPhysName; mGL->glGenBuffers(1, &newPhysName);
        refreshName(ObjectType::BUFFER, bufferName, newPhysName);

        const GLBufferState& bufferState = mBuffers[bufferName];

        if (!bufferState.size) continue;

        // It's OK; restore buffer binding state later
        mGL->glBindBuffer(GL_ARRAY_BUFFER, newPhysName);
        mGL->glBufferData(GL_ARRAY_BUFFER, bufferState.size, bufferState.data, bufferState.usage);
        mGL->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    mGL->glActiveTexture(GL_TEXTURE0);

    // Texture objects (must be before FBOs in case they use a texture-backed FBO)
    for (auto& it: currNames[ObjectType::TEXTURE]) {
        GLuint textureName = it.first;

        GLuint newPhysName; mGL->glGenTextures(1, &newPhysName);
        refreshName(ObjectType::TEXTURE, textureName, newPhysName);

        const GLTextureState& textureState = mTextures[textureName];

        mGL->glBindTexture(textureState.boundTarget, newPhysName);
        mGL->glPixelStorei(GL_UNPACK_ALIGNMENT, textureState.unpackAlignment);
        for (int level = 0; level < SNAPSHOT_MAX_TEXTURE_LEVELS; level++) {
            if (!textureState.data[level]) continue;
            mGL->glTexImage2D(
                    textureState.boundTarget,
                    level,
                    textureState.internalformat,
                    textureState.width[level],
                    textureState.height[level],
                    0,
                    textureState.format,
                    textureState.type,
                    textureState.data[level]);
        }
        mGL->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        mGL->glBindTexture(textureState.boundTarget, 0);
    }

    // TODO: Texture binding state w.r.t all texture units
    // In boot, only GL_TEXTURE0 is used.

    // FBO
    for (auto& it: currNames[ObjectType::FRAMEBUFFER]) {
        GLuint fboName = it.first;

        GLuint newPhysName; mGL->glGenFramebuffers(1, &fboName);
        refreshName(ObjectType::FRAMEBUFFER, fboName, newPhysName);

        const GLFBOState& fboState = mFBOs[fboName];

        mGL->glBindFramebuffer(GL_FRAMEBUFFER, newPhysName);
        for (const auto& jt : fboState.attachments) {
            GLenum attachment = jt.first;

            const GLFBOAttachmentInfo& info = jt.second;

            switch (info.type) {
            case GL_TEXTURE_2D:
                mGL->glFramebufferTexture2D(
                        GL_FRAMEBUFFER,
                        attachment,
                        info.texture.textarget,
                        toPhysName(ObjectType::TEXTURE, info.texture.texture),
                        info.texture.level);
            case GL_RENDERBUFFER:
            default:
                break;
            }
        }
        mGL->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // TODO: FBO binding state w.r.t. all FBO targets
    // In boot, only GL_FRAMEBUFFER is used.

    GLenum blendRGBSrc = GL_ONE;
    GLenum blendRGBDst = GL_ONE;

    GLenum blendAlphaSrc = GL_ONE;
    GLenum blendAlphaDst = GL_ONE;

    for (const auto& it : mGlobals) {
        const GLValue& val = it.second;
        std::vector<float> floats = val.floats;
        switch (it.first) {
        case GL_COLOR_CLEAR_VALUE:
            mGL->glClearColor(floats[0], floats[1], floats[2], floats[3]);
            break;
        case GL_ACTIVE_TEXTURE:
            mGL->glActiveTexture(val.ints[0]);
            break;
        case GL_CURRENT_PROGRAM:
            mGL->glUseProgram(val.ints[0]);
            break;
        case GL_ARRAY_BUFFER_BINDING:
            mGL->glBindBuffer(GL_ARRAY_BUFFER, val.ints[0]);
            break;
        case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            mGL->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, val.ints[0]);
            break;
        case GL_FRAMEBUFFER_BINDING:
            mGL->glBindFramebuffer(GL_FRAMEBUFFER, val.ints[0]);
            break;
        case GL_BLEND_SRC_RGB:
            blendRGBSrc = val.ints[0];
            break;
        case GL_BLEND_SRC_ALPHA:
            blendAlphaSrc = val.ints[0];
            break;
        case GL_BLEND_DST_RGB:
            blendRGBDst = val.ints[0];
            break;
        case GL_BLEND_DST_ALPHA:
            blendAlphaDst = val.ints[0];
            break;
        case GL_VIEWPORT:
            mGL->glViewport(val.ints[0], val.ints[1], val.ints[2], val.ints[3]);
            break;
        case GL_SCISSOR_BOX:
            mGL->glScissor(val.ints[0], val.ints[1], val.ints[2], val.ints[3]);
            break;
        default:
            break;
        }
    }

    if (blendRGBSrc != GL_ONE || blendRGBDst != GL_ONE)
        mGL->glBlendFunc(blendRGBSrc, blendRGBDst);

    if (blendAlphaSrc != GL_ONE || blendAlphaDst != GL_ONE)
        mGL->glBlendFunc(blendAlphaSrc, blendAlphaDst);
}

GLuint GLSnapshotState::glCreateShader(GLuint shader, GLenum shaderType) {
    GLuint res = genName(ObjectType::PROGRAM, shader);
    mShaders[res].type = shaderType;
    return res;
}

GLuint GLSnapshotState::glCreateProgram(GLuint program) {
    return genName(ObjectType::PROGRAM, program, true);
}

void GLSnapshotState::glGenBuffers(GLsizei n, GLuint* buffers) {
    genMulti(ObjectType::BUFFER, n, buffers, buffers);
}

void GLSnapshotState::glGenFramebuffers(GLsizei n, GLuint* fbos) {
    genMulti(ObjectType::FRAMEBUFFER, n, fbos, fbos);
}

void GLSnapshotState::glGenTextures(GLsizei n, GLuint* textures) {
    genMulti(ObjectType::TEXTURE, n, textures, textures);
}

void GLSnapshotState::glDeleteShader(GLuint shader) {
    cleanupName(ObjectType::PROGRAM, shader);
}

void GLSnapshotState::glDeleteProgram(GLuint program) {
    cleanupName(ObjectType::PROGRAM, program);
}

void GLSnapshotState::glDeleteBuffers(GLsizei n, GLuint* buffers) {
    cleanupMulti(ObjectType::BUFFER, n, buffers);
}

void GLSnapshotState::glDeleteFramebuffers(GLsizei n, GLuint* fbos) {
    cleanupMulti(ObjectType::FRAMEBUFFER, n, fbos);
}

void GLSnapshotState::glDeleteTextures(GLsizei n, GLuint* textures) {
    cleanupMulti(ObjectType::TEXTURE, n, textures);
}

GLuint GLSnapshotState::getName(GLSnapshotState::ObjectType type, GLuint name) {
    return mNames[type][name];
}

void GLSnapshotState::glShaderString(GLuint shader, const GLchar* string) {
    mShaders[toVirtName(ObjectType::PROGRAM, shader)].source = std::string(string);
}

void GLSnapshotState::glAttachShader(GLuint program, GLuint shader) {
    GLuint program_v = toVirtName(ObjectType::PROGRAM, program);
    GLuint shader_v = toVirtName(ObjectType::PROGRAM, shader);
    GLenum shaderType = mShaders[shader_v].type;
    mPrograms[program_v].linkage[shaderType] = shader_v;
}

void GLSnapshotState::glCompileShader(GLuint shader) {
    GLuint shader_v = toVirtName(ObjectType::PROGRAM, shader);
    mShaders[shader_v].compileStatus = true;
}

void GLSnapshotState::glLinkProgram(GLuint program) {
    GLuint program_v = toVirtName(ObjectType::PROGRAM, program);
    mPrograms[program_v].linkStatus = true;
}

void GLSnapshotState::glUseProgram(GLuint program) {
    mCurrentProgram = toVirtName(ObjectType::PROGRAM, program);
}

void GLSnapshotState::glBindAttribLocation(GLuint program, GLint index, const GLchar* name) {
    GLuint program_v = toVirtName(ObjectType::PROGRAM, program);
    mPrograms[program_v].boundAttribLocs[index] = name;
}

void GLSnapshotState::glUniform1i(GLint location, GLint x) {
    GLValue val; val.ints.resize(1); val.ints[0] = x;
    saveUniform(location, 0, GL_FALSE, GLValueType::VALTYPE_1I, val);
}

void GLSnapshotState::glUniform1f(GLint location, GLfloat x) {
    GLValue val; val.floats.resize(1); val.floats[0] = x;
    saveUniform(location, 0, GL_FALSE, GLValueType::VALTYPE_1F, val);
}

void GLSnapshotState::glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GLValue val; val.floats.resize(4);
    val.floats[0] = x; val.floats[1] = y; val.floats[2] = z; val.floats[3] = w;
    saveUniform(location, 0, GL_FALSE, GLValueType::VALTYPE_4F, val);
}

void GLSnapshotState::glUniform4fv(GLint location, GLsizei count, const GLfloat* vals) {
    GLValue val; val.floats.resize(count * 4);
    memcpy(&val.floats[0], vals, count * 4 * sizeof(float));
    saveUniform(location, count, GL_FALSE, GLValueType::VALTYPE_4FV, val);
}

void GLSnapshotState::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLValue val; val.floats.resize(count * 16);
    memcpy(&val.floats[0], value, count * 16 * sizeof(float));
    saveUniform(location, count, transpose, GLValueType::VALTYPE_M4FV, val);
}

void GLSnapshotState::glBindBuffer(GLenum target, GLuint buffer) {
    GLuint buffer_v = toVirtName(ObjectType::BUFFER, buffer);
    mBufferBindings[target] = buffer_v;
}

void GLSnapshotState::glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
    GLuint buffer_v = mBufferBindings[target];
    GLBufferState& buf = mBuffers[buffer_v];
    buf.data = ::realloc(buf.data, size);
    memcpy(buf.data, data, size);
    buf.size = size;
    buf.usage = usage;
}

void GLSnapshotState::glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
    GLuint buffer_v = mBufferBindings[target];
    GLBufferState& buf = mBuffers[buffer_v];
    memcpy(((char*)buf.data) + offset, data, size);
}

void GLSnapshotState::glBindTexture(GLenum target, GLuint texture) {
    GLuint texture_v = toVirtName(ObjectType::TEXTURE, texture);
    mTextureBindings[mActiveTexture][target] = texture_v;
}

void GLSnapshotState::glActiveTexture(GLenum unit) {
    mActiveTexture = unit;
}

void GLSnapshotState::glPixelStorei(GLenum pname, GLint param) {
    switch (pname) {
    case GL_PACK_ALIGNMENT:
        mPackAlignment = param;
        break;
    case GL_UNPACK_ALIGNMENT:
        mUnpackAlignment = param;
        break;
    // TODO: other pixel store enums
    default:
        break;
    }
}

static uint32_t sAlign(uint32_t v, uint32_t align) {

    uint32_t rem = v % align;
    return rem ? (v + (align - rem)) : v;

}

static uint32_t sTexPixelSize(GLenum internalformat,
                              GLenum format,
                              GLenum type) {
    uint32_t reps = 3;
    switch (internalformat) {
    case GL_ALPHA:
        reps = 1;
        break;
    case GL_RGB:
        if (type == GL_UNSIGNED_SHORT_5_6_5)
            reps = 1;
        else
            reps = 3;
        break;
    case GL_RGBA:
        reps = 4;
        break;
    default:
        break;
    }

    uint32_t eltSize = 1;

    switch (type) {
    case GL_UNSIGNED_SHORT_5_6_5:
        eltSize = 2;
        break;
    case GL_UNSIGNED_BYTE:
        eltSize = 1;
        break;
    default:
        break;
    }

    uint32_t pixelSize = reps * eltSize;

    return pixelSize;
}

static uint32_t sTexImageSize(GLenum internalformat,
                              GLenum format,
                              GLenum type,
                              int unpackAlignment,
                              GLsizei width, GLsizei height) {

    uint32_t alignedWidth = sAlign(width, unpackAlignment);
    uint32_t pixelSize = sTexPixelSize(internalformat, format, type);
    uint32_t totalSize = pixelSize * alignedWidth * height;

    return totalSize;
}

void GLSnapshotState::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {

    GLuint texture_v = mTextureBindings[mActiveTexture][target];
    GLTextureState& tex = mTextures[texture_v];

    uint32_t totalSize = sTexImageSize(internalformat, format, type, mUnpackAlignment, width, height);

    tex.nbytes[level] = totalSize;

    if (pixels) {
        tex.data[level] = realloc(tex.data[level], totalSize);
        memcpy(tex.data[level], pixels, totalSize);
    }

    tex.width[level] = width;
    tex.height[level] = height;

    tex.internalformat = internalformat;
    tex.format = format;
    tex.type = type;

    tex.unpackAlignment = mUnpackAlignment;
}

void GLSnapshotState::glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) {

    GLuint texture_v = mTextureBindings[mActiveTexture][target];
    GLTextureState& tex = mTextures[texture_v];

    uint32_t srcPixelSize = sTexPixelSize(format, format, type);
    uint32_t dstPixelSize = sTexPixelSize(tex.internalformat, tex.format, tex.type);

    uint32_t alignedSrcWidth = sAlign(width, mUnpackAlignment) * srcPixelSize;
    uint32_t alignedDstWidth = sAlign(tex.width[level], mUnpackAlignment) * dstPixelSize;

    if (pixels) {
        char* dst = (char*)tex.data[level];
        const char* src = (const char*)pixels;

        for (int y = 0; y < height; y++) {
            char* dstRowStart = dst + (yoffset + y) * alignedDstWidth + xoffset * dstPixelSize;
            const char* srcRowStart = src + y * alignedSrcWidth;
            memcpy(dstRowStart, srcRowStart, alignedSrcWidth);
        }
    }
}

void GLSnapshotState::glTexParameteri(GLenum target, GLenum pname, GLint param) {
    GLuint texture_v = mTextureBindings[mActiveTexture][target];
    GLTextureState& tex = mTextures[texture_v];

    switch (pname) {
    case GL_TEXTURE_MAG_FILTER:
        tex.magFilter = param;
        break;
    case GL_TEXTURE_MIN_FILTER:
        tex.minFilter = param;
        break;
    case GL_TEXTURE_WRAP_S:
        tex.wrapS = param;
        break;
    case GL_TEXTURE_WRAP_T:
        tex.wrapT = param;
        break;
    default:
        break;
    }
}

void GLSnapshotState::glBindFramebuffer(GLenum target, GLuint framebuffer) {
    GLuint framebuffer_v = toVirtName(ObjectType::FRAMEBUFFER, framebuffer);
    mFBOBindings[target] = framebuffer_v;
}

void GLSnapshotState::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    GLuint fbo_v = mFBOBindings[target];

    GLFBOState& fbo = mFBOs[fbo_v];

    GLFBOAttachmentInfo info;
    info.type = GL_TEXTURE_2D;
    info.texture.textarget = textarget;
    info.texture.texture = texture;
    info.texture.level = level;

    fbo.attachments[target] = info;
}

void GLSnapshotState::glEnableVertexAttribArray(GLuint index) {
    mVAOs[mCurrVAO].attribs[index].enabled = true;
}

void GLSnapshotState::glDisableVertexAttribArray(GLuint index) {
    mVAOs[mCurrVAO].attribs[index].enabled = false;
}

void GLSnapshotState::glVertexAttribPointerOffset(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint offset) {

    mVAOs[mCurrVAO].attribs[index].size = size;
    mVAOs[mCurrVAO].attribs[index].type = type;
    mVAOs[mCurrVAO].attribs[index].normalized = normalized;
    mVAOs[mCurrVAO].attribs[index].stride = stride;

    mVAOs[mCurrVAO].attribs[index].arrayBuffer = mBufferBindings[GL_ARRAY_BUFFER];

    mVAOs[mCurrVAO].attribs[index].offset = offset;
}

void GLSnapshotState::glVertexAttribPointerData(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void* data, GLuint datalen) {

    mVAOs[mCurrVAO].attribs[index].size = size;
    mVAOs[mCurrVAO].attribs[index].type = type;
    mVAOs[mCurrVAO].attribs[index].normalized = normalized;
    mVAOs[mCurrVAO].attribs[index].stride = stride;

    mVAOs[mCurrVAO].attribs[index].arrayBuffer = 0;

    void* dataDst = (void*)mVAOs[mCurrVAO].attribs[index].data;
    dataDst = realloc(dataDst, datalen);
    memcpy((char*)dataDst, data, datalen);

    mVAOs[mCurrVAO].attribs[index].data = dataDst;
}

} // namespace GLSnapshot
