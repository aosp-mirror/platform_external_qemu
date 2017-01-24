#pragma once

#include "GLESv2Dispatch.h"

#include <map>
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

typedef std::map<GLenum, GLValue> GlobalStateMap;

class GLSnapshotState {
public:
    GLSnapshotState(const GLESv2Dispatch* gl);
    void save();
    void restore();
private:
    void getGlobalStateEnum(GLenum name, int size);
    void getGlobalStateByte(GLenum name, int size);
    void getGlobalStateInt(GLenum name, int size);
    void getGlobalStateFloat(GLenum name, int size);
    void getGlobalStateInt64(GLenum name, int size);

    const GLESv2Dispatch* mGL;
    GlobalStateMap mGlobals;
};

} // namespace GLSnapshot
