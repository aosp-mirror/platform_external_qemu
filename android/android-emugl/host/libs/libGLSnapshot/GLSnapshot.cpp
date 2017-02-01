#include "GLSnapshot.h"

#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

#include <stdio.h>
#include <stdlib.h>

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

    // Clear color, blend func, depth range
    getGlobalStateFloat(GL_COLOR_CLEAR_VALUE, 4);

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

    // Texture objects (must be before FBOs in case they use a texture-backed FBO)
    for (auto& it: currNames[ObjectType::TEXTURE]) {
        GLuint textureName = it.first;

        GLuint newPhysName; mGL->glGenTextures(1, &newPhysName);
        refreshName(ObjectType::TEXTURE, textureName, newPhysName);

        // mGL->glTexImage2D(GL_TEXTURE_2D, 0, 
        // TODO: restore texparameter
    }

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
            mGL->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_BINDING, val.ints[0]);
            break;
        default:
            break;
        }
    }

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
    mTextureBindings[target] = texture_v;
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

    GLuint texture_v = mTextureBindings[target];
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
}

void GLSnapshotState::glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) {

    GLuint texture_v = mTextureBindings[target];
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
    GLuint texture_v = mTextureBindings[target];
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

} // namespace GLSnapshot
