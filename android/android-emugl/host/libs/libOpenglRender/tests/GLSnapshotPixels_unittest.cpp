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

class SnapshotGlUnpackAlignmentTest
    : public SnapshotSetValueTest<GLuint>,
      public ::testing::WithParamInterface<GLuint> {
    void stateCheck(GLuint expected) override {
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_UNPACK_ALIGNMENT, expected));
    }
    void stateChange() override {
        gl->glPixelStorei(GL_UNPACK_ALIGNMENT, *m_changed_value);
    }
};

TEST_P(SnapshotGlUnpackAlignmentTest, SetUnpackAlign) {
    setExpectedValues(4, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixels,
                        SnapshotGlUnpackAlignmentTest,
                        ::testing::Values(1, 2, 4, 8));

class SnapshotGlPackAlignmentTest
    : public SnapshotSetValueTest<GLuint>,
      public ::testing::WithParamInterface<GLuint> {
    void stateCheck(GLuint expected) override {
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_PACK_ALIGNMENT, expected));
    }
    void stateChange() override {
        gl->glPixelStorei(GL_PACK_ALIGNMENT, *m_changed_value);
    }
};

TEST_P(SnapshotGlPackAlignmentTest, SetPackAlign) {
    setExpectedValues(4, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixels,
                        SnapshotGlPackAlignmentTest,
                        ::testing::Values(1, 2, 4, 8));

}  // namespace emugl
