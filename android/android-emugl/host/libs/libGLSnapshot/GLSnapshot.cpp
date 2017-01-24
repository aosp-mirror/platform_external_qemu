#include "GLSnapshot.h"

#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

#include <stdio.h>

#define DEBUG 1

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

void GLSnapshotState::save() {
    getGlobalStateFloat(GL_COLOR_CLEAR_VALUE, 4);
    getGlobalStateInt(GL_ACTIVE_TEXTURE, 1);
}

void GLSnapshotState::restore() {
    std::vector<float> clearColor = mGlobals[GL_COLOR_CLEAR_VALUE].floats;
    mGL->glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

    mGL->glActiveTexture(mGlobals[GL_ACTIVE_TEXTURE].ints[0]);
}

} // namespace GLSnapshot
