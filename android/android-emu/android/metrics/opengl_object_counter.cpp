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
#include "android/metrics/opengl_object_counter.h"

#include "android/base/memory/LazyInstance.h"

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
    int counter[toIndex(NamedObjectType::NUM_OBJECT_TYPES)];
};

static android::base::LazyInstance<OpenGLObjectCounter> sGlobal;

void opengl_object_count_inc(int type) {
    if (type < toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        sGlobal->counter[type] += 1;
    }
}
void opengl_object_count_dec(int type) {
    if (type < toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        sGlobal->counter[type] -= 1;
    }
}

std::vector<int> get_opengl_object_counts() {
    return std::vector<int>(
            sGlobal->counter,
            sGlobal->counter + toIndex(NamedObjectType::NUM_OBJECT_TYPES));
}
