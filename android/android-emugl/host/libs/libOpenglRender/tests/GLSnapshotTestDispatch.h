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

#pragma once

#include "emugl/common/OpenGLDispatchLoader.h"

#include "android/base/testing/TestSystem.h"

namespace emugl {

// Global dispatch object with functions overridden for snapshot testing
const GLESv2Dispatch* getSnapshotTestDispatch();

// SnapshotTestDispatch - a GL dispatcher with some of its functions overridden
// that can act as a drop-in replacement for GLESv2Dispatch. These functions are
// given wrappers that perform a test of the GL snapshot when they are called.
//
// It uses the FrameBuffer to perform the snapshot; thus will only work in an
// environment where the FrameBuffer exists.
class SnapshotTestDispatch : public GLESv2Dispatch {
public:
    SnapshotTestDispatch();

protected:
    void overrideFunctions();

    void saveSnapshot();

    void loadSnapshot();

    static void testDraw(std::function<void()> doDraw);

    static void test_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
        testDraw([&] {
            LazyLoadedGLESv2Dispatch::get()->glDrawArrays(mode, first, count);
        });
    }

    static void test_glDrawElements(GLenum mode,
                                    GLsizei count,
                                    GLenum type,
                                    const GLvoid* indices) {
        testDraw([&] {
            LazyLoadedGLESv2Dispatch::get()->glDrawElements(mode, count, type,
                                                            indices);
        });
    }

    bool mValid = false;

    int mLoadCount = 0;

    android::base::TestSystem mTestSystem;
    std::string mSnapshotPath = {};
    std::string mSnapshotFile = {};
    std::string mTextureFile = {};
};

}  // namespace emugl
