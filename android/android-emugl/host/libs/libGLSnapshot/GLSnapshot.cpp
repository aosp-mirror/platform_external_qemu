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

void GLSnapshotState::save() {
    D("save clear color");
    mEnums[GL_COLOR_CLEAR_VALUE].floats.resize(4);
    mGL->glGetFloatv(GL_COLOR_CLEAR_VALUE, &mEnums[GL_COLOR_CLEAR_VALUE].floats[0]);
}

void GLSnapshotState::restore() {
    D("restore clear color");
    std::vector<float> clearColor = mEnums[GL_COLOR_CLEAR_VALUE].floats;
    mGL->glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
}

} // namespace GLSnapshot
