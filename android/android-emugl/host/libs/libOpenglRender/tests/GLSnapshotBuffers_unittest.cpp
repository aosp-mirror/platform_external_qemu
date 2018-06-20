// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GLSnapshotTesting.h"
#include "OpenGLTestContext.h"

#include <gtest/gtest.h>

namespace emugl {

struct GlBufferData {
    GLenum target;
    GLsizeiptr size;
    GLvoid* data;
    GLenum usage;
};

class SnapshotGlBufferObjectsTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        // TODO(benzene): m_buffer_name should be default or uninitialized
    }

    void stateChange() {
        // TODO(benzene): gen buffer, change some stuff?
    }

protected:
    GLuint m_buffer_name;
};

TEST_F(SnapshotGlBufferObjectsTest, FooBarBuffers) {
    doCheckedSnapshot();
}

}  // namespace emugl
