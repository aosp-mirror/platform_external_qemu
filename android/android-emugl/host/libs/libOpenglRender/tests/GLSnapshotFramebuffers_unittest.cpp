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

#include <gtest/gtest.h>

namespace emugl {

class SnapshotGlFramebufferTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_EQ(GL_FALSE, gl->glIsFramebuffer(m_framebuffer_name));
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_FRAMEBUFFER_BINDING, 0));
    }

    void changedStateCheck() override {
        EXPECT_EQ(GL_TRUE, gl->glIsFramebuffer(m_framebuffer_name));
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_FRAMEBUFFER_BINDING,
                                       m_framebuffer_name));
    }

    void stateChange() override {
        gl->glGenFramebuffers(1, &m_framebuffer_name);
        gl->glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_name);

        m_state_changer();
    }

    void setStateChanger(std::function<void()> changer) {
        m_state_changer = changer;
    }

protected:

    GLuint m_framebuffer_name = 0;
    std::function<void()> m_state_changer = [] {};
};

TEST_F(SnapshotGlFramebufferTest, CreateAndBind) {
    doCheckedSnapshot();
}

}  // namespace emugl
