#pragma once

#include "GLESv2Dispatch.h"

#include <array>
#include <unordered_map>
#include <string>
#include <vector>

#include <inttypes.h>

#include <GLES2/gl2.h>

namespace GLSnapshot {

struct GLValue {
    std::vector<GLenum> enums;
    std::vector<unsigned char> bytes;
    std::vector<uint16_t> shorts;
    std::vector<uint32_t> ints;
    std::vector<float> floats;
    std::vector<uint64_t> int64s;
};

typedef std::unordered_map<GLenum, GLValue> GlobalStateMap;
typedef std::unordered_map<GLenum, bool> GlobalEnables;

struct GLShaderState {
    GLenum type = GL_NONE;
    std::string source = "";
    bool compileStatus = false;
};

struct GLProgramState {
    std::unordered_map<GLenum, GLuint> linkage = {};
    bool linkStatus = false;
};

struct GLBufferState {
    void* data = nullptr;
    uint64_t size = 0;;
    GLenum usage = GL_NONE;
    bool mapped = false;
    uint32_t mappedAccess = 0;;
    uint64_t mappedOffset = 0;;
    uint64_t mappedLength = 0;;
};

struct GLFBOState {
    std::unordered_map<GLenum, GLuint> attachments;
    std::unordered_map<GLenum, GLenum> attachmentTypes;
};

// level 0 only for now
// client arrays only for now
#define SNAPSHOT_MAX_TEXTURE_LEVELS 14

struct GLTextureState {
    GLTextureState() {
        memset((void**)data, 0, sizeof(void*) * SNAPSHOT_MAX_TEXTURE_LEVELS);
        memset(nbytes, 0, sizeof(uint64_t) * SNAPSHOT_MAX_TEXTURE_LEVELS);
        memset(width, 0, sizeof(GLuint) * SNAPSHOT_MAX_TEXTURE_LEVELS);
        memset(height, 0, sizeof(GLuint) * SNAPSHOT_MAX_TEXTURE_LEVELS);
    }
    void* data[SNAPSHOT_MAX_TEXTURE_LEVELS];
    uint64_t nbytes[SNAPSHOT_MAX_TEXTURE_LEVELS];
    GLuint width[SNAPSHOT_MAX_TEXTURE_LEVELS];
    GLuint height[SNAPSHOT_MAX_TEXTURE_LEVELS];
    GLenum internalformat = GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;

    GLint magFilter;
    GLint minFilter;
    GLint wrapS;
    GLint wrapT;
};

class GLSnapshotState {
public:
    enum ObjectType {
        PROGRAM = 0,
        BUFFER = 1,
        FRAMEBUFFER = 2,
        RENDERBUFFER = 3,
        TEXTURE = 4,
        VERTEXARRAYOBJECT = 5,
        TRANSFORMFEEDBACK = 6,
        SAMPLER = 7,
        QUERY = 8,
        NUM_OBJECT_TYPES = 9,
    };
    GLSnapshotState(const GLESv2Dispatch* gl);
    void save();
    void restore();

    GLuint getName(ObjectType type, GLuint name);

    // Gen/deletes
    GLuint glCreateShader(GLuint shader, GLenum shaderType);
    GLuint glCreateProgram(GLuint program);
    void glGenBuffers(GLsizei n, GLuint* buffers);
    void glGenFramebuffers(GLsizei n, GLuint* fbos);
    void glGenTextures(GLsizei n, GLuint* textures);

    void glDeleteShader(GLuint shader);
    void glDeleteProgram(GLuint program);
    void glDeleteBuffers(GLsizei n, GLuint* buffers);
    void glDeleteFramebuffers(GLsizei n, GLuint* fbox);
    void glDeleteTextures(GLsizei n, GLuint* textures);

    // Shader state
    void glShaderString(GLuint shader, const GLchar* string);
    void glAttachShader(GLuint program, GLuint shader);
    void glCompileShader(GLuint shader);
    void glLinkProgram(GLuint shader);

    // Buffer state
    std::unordered_map<GLenum, GLuint> mBufferBindings;
    void glBindBuffer(GLenum target, GLuint buffer);
    void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
    void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);

    // Texture state
    uint32_t mPackAlignment = 4;
    uint32_t mUnpackAlignment = 4;
    uint32_t mActiveTexture = GL_TEXTURE0;
    std::unordered_map<GLenum, GLuint> mTextureBindings;
    void glBindTexture(GLenum target, GLuint texture);
    void glActiveTexture(GLenum param);
    void glPixelStorei(GLenum pname, GLint param);
    void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
    void glTexParameteri(GLenum target, GLenum pname, GLint param);

private:

    using GLNameMap = std::unordered_map<GLuint, GLuint>;
    using GLTypedNameMap = std::array<GLNameMap, ObjectType::NUM_OBJECT_TYPES>;

    void getGlobalStateEnum(GLenum name, int size);
    void getGlobalStateByte(GLenum name, int size);
    void getGlobalStateInt(GLenum name, int size);
    void getGlobalStateFloat(GLenum name, int size);
    void getGlobalStateInt64(GLenum name, int size);

    void getGlobalStateEnable(GLenum name);

    GLuint toPhysName(ObjectType, GLuint name);
    GLuint toVirtName(ObjectType, GLuint name);

    const GLESv2Dispatch* mGL;
    GlobalStateMap mGlobals;
    GlobalEnables mEnables;

    // |mNames| maps virtual GL names to "physical" GL names
    // (i.e., those used by the underlying GLES implementation).
    // Virtual GL names stay constant across snapshots.
    GLTypedNameMap mNames;
    GLTypedNameMap mNamesBack; // Map from physical to virtual.

    GLuint mNextName[ObjectType::NUM_OBJECT_TYPES] = {};

    GLuint nextName(ObjectType type) {
        return mNextName[type]++;
    }

    GLuint genName(ObjectType type, GLuint p, bool isProgram = false) {
        GLuint v = nextName(type);
        mNames[type][v] = p;
        mNamesBack[type][p] = v;
        switch (type) {
            // shaders/programs should never share the same name.
            case ObjectType::PROGRAM:
                if (isProgram)
                    mPrograms[v] = GLProgramState();
                else
                    mShaders[v] = GLShaderState();
                break;
            case ObjectType::BUFFER:
                mBuffers[v] = GLBufferState();
                break;
            case ObjectType::FRAMEBUFFER:
                mFBOs[v] = GLFBOState();
                break;
            case ObjectType::TEXTURE:
                mTextures[v] = GLTextureState();
                break;
            default:
                break;
        }
        return v;
    }

    void genMulti(ObjectType type, GLsizei n, GLuint* in, GLuint* out) {
        for (int i = 0; i < n; i++)
            out[i] = genName(type, in[i]);
    }

    void cleanupName(ObjectType type, GLuint p) {
        GLuint v = toVirtName(type, p);
        mNames[type].erase(v);
        mNamesBack[type].erase(p);
        switch (type) {
            // shaders/programs should never share the same name.
            case ObjectType::PROGRAM:
                mShaders.erase(v);
                mPrograms.erase(v);
                break;
            case ObjectType::BUFFER:
                mBuffers.erase(v);
                break;
            case ObjectType::FRAMEBUFFER:
                mFBOs.erase(v);
                break;
            case ObjectType::TEXTURE:
                mTextures.erase(v);
                break;
            default:
                break;
        }
    }

    void cleanupMulti(ObjectType type, GLsizei n, GLuint* todo) {
        for (int i = 0; i < n; i++)
            cleanupName(type, todo[i]);
    }

    void refreshName(ObjectType type, GLuint v, GLuint newp) {
        mNamesBack[type].erase(toPhysName(type, v));
        mNames[type].erase(v);
        mNames[type][v] = newp;
        mNamesBack[type][newp] = v;
    }

    std::unordered_map<GLuint, GLShaderState> mShaders;
    std::unordered_map<GLuint, GLProgramState> mPrograms;
    std::unordered_map<GLuint, GLBufferState> mBuffers;
    std::unordered_map<GLuint, GLFBOState> mFBOs;
    std::unordered_map<GLuint, GLTextureState> mTextures;
};

} // namespace GLSnapshot
