/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License")
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include "android/base/Compiler.h"

#include <GLES/gl.h>

#include <unordered_map>
#include <vector>

// Convenience class for the translator to perform some kind of
// workaround or emulation that touches a lot of GL state,
// like drawing fullscreen quads.
class ScopedGLState {
public:
    ScopedGLState() = default;
    void push(GLenum name);
    void push(std::initializer_list<GLenum> names);
    void pushForCoreProfileTextureEmulation();
    ~ScopedGLState();
private:
    struct StateVector {
        union {
            int intData[4];
            float floatData[4];
        };
    };

    std::unordered_map<GLenum, StateVector> mStateMap;
    DISALLOW_COPY_AND_ASSIGN(ScopedGLState);
};
