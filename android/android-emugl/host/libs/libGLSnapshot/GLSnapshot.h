#pragma once

#include "GLESv2Dispatch.h"

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

    // Gen/deletes
    GLuint glCreateShader(GLuint shader, GLenum shaderType);
    GLuint glCreateProgram(GLuint program);

    void glGenBuffers(GLsizei n, GLuint* buffers);

    // Shader state
    void glShaderString(GLuint shader, const GLchar* string);
    void glAttachShader(GLuint program, GLuint shader);

    GLuint getName(ObjectType type, GLuint name);

private:

    using GLNameMap = std::unordered_map<GLuint, GLuint>;
    using GLTypedNameMap = std::unordered_map<unsigned int, GLNameMap>;

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

    GLuint mNextProgramName = 1;
    GLuint mNextBufferName = 1;
    GLuint mNextFramebufferName = 1;
    GLuint mNextRenderbufferName = 1;
    GLuint mNextTextureName = 1;
    GLuint mNextVertexArrayObjectName = 1;
    GLuint mNextTransformFeedbackName = 1;
    GLuint mNextSamplerName = 1;
    GLuint mNextQueryName = 1;

    GLuint nextName(ObjectType type) {
        switch (type) {
            case ObjectType::PROGRAM:
                return mNextProgramName++;
            case ObjectType::BUFFER:
                return mNextBufferName++;
            case ObjectType::FRAMEBUFFER:
                return mNextFramebufferName++;
            case ObjectType::RENDERBUFFER:
                return mNextRenderbufferName++;
            case ObjectType::TEXTURE:
                return mNextTextureName++;
            case ObjectType::VERTEXARRAYOBJECT:
                return mNextVertexArrayObjectName++;
            case ObjectType::TRANSFORMFEEDBACK:
                return mNextTransformFeedbackName++;
            case ObjectType::SAMPLER:
                return mNextSamplerName++;
            case ObjectType::QUERY:
                return mNextQueryName++;
            default:
                return 0;
        }
    }
    void remapName(ObjectType type, GLuint v, GLuint p) {
        mNames[type][v] = p;
        mNamesBack[type][p] = v;
    }
    void unmapName(ObjectType type, GLuint v, GLuint p) {
        mNames[type].erase(v);
        mNamesBack[type].erase(p);
    }

    std::unordered_map<GLuint, GLShaderState> mShaderState;
    std::unordered_map<GLuint, GLProgramState> mProgramState;
};

} // namespace GLSnapshot
