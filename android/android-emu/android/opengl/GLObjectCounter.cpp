// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/opengl/GLObjectCounter.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

using android::base::AutoLock;
using android::base::Lock;

enum class NamedObjectType : short {
    NULLTYPE,
    VERTEXBUFFER,
    TEXTURE,
    RENDERBUFFER,
    FRAMEBUFFER,
    SHADER_OR_PROGRAM,
    SAMPLER,
    QUERY,
    VERTEX_ARRAY_OBJECT,
    NUM_OBJECT_TYPES  // Must be last
};

static constexpr int toIndex(NamedObjectType type) {
    return static_cast<int>(type);
}

struct OpenGLObjectCounter {
    Lock mLock;
    int mCounter[toIndex(NamedObjectType::NUM_OBJECT_TYPES)]{0};
};

static android::base::LazyInstance<OpenGLObjectCounter> sGlobal;

void opengl_object_count_inc(int type) {
    if (type > toIndex(NamedObjectType::NULLTYPE) &&
        type < toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        AutoLock lock(sGlobal->mLock);
        sGlobal->mCounter[type] += 1;
    }
}
void opengl_object_count_dec(int type) {
    if (type > toIndex(NamedObjectType::NULLTYPE) &&
        type < toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        AutoLock lock(sGlobal->mLock);
        sGlobal->mCounter[type] -= 1;
    }
}
namespace android {
namespace opengl {

void getOpenGLObjectCounts(std::vector<int>* vec) {
    AutoLock lock(sGlobal->mLock);
    *vec = std::vector<int>(std::begin(sGlobal->mCounter),
                            std::end(sGlobal->mCounter));
}

}  // namespace opengl
}  // namespace android