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
};

typedef std::map<GLenum, GLValue> StateEnumMap;

class GLSnapshotState {
public:
    GLSnapshotState(const GLESv2Dispatch* gl);
    void save();
    void restore();
private:
    const GLESv2Dispatch* mGL;
    StateEnumMap mEnums;
};

} // namespace GLSnapshot
