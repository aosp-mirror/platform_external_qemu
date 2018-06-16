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

TEST_F(SnapshotTest, InitDestroy) {}

class SnapshotGlEnableTest : public SnapshotPreserveTest,
                             public ::testing::WithParamInterface<GLenum> {
    void defaultStateCheck() override {
        EXPECT_FALSE(gl->glIsEnabled(GetParam()));
    }
    void changedStateCheck() override {
        EXPECT_TRUE(gl->glIsEnabled(GetParam()));
    }
    void stateChange() override { gl->glEnable(GetParam()); }
};

TEST_P(SnapshotGlEnableTest, PreserveEnable) {
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotCapability,
                        SnapshotGlEnableTest,
                        ::testing::ValuesIn(kGLES2CanBeEnabled));

class SnapshotGlDisableTest : public SnapshotPreserveTest,
                              public ::testing::WithParamInterface<GLenum> {
    void defaultStateCheck() override {
        EXPECT_TRUE(gl->glIsEnabled(GetParam()));
    }
    void changedStateCheck() override {
        EXPECT_FALSE(gl->glIsEnabled(GetParam()));
    }
    void stateChange() override { gl->glDisable(GetParam()); }
};

TEST_P(SnapshotGlDisableTest, PreserveDisable) {
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotCapability,
                        SnapshotGlDisableTest,
                        ::testing::ValuesIn(kGLES2CanBeDisabled));

}  // namespace emugl
