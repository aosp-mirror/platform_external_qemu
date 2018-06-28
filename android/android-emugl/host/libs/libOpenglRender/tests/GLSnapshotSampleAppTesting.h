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

#include "GLSnapshotTesting.h"
#include "Standalone.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

#include <gtest/gtest.h>

namespace emugl {

using android::base::LazyInstance;
using android::base::StdioStream;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

class SnapshotTestApplication {
public:
    SnapshotTestApplication(
            std::function<void()> initFunc = []{},
            std::function<void()> drawFunc = []{})
        : mTestSystem(PATH_SEP "progdir",
                      android::base::System::kProgramBitness,
                      PATH_SEP "homedir",
                      PATH_SEP "appdir") {
        mDrawFunc = drawFunc;
        mTestSystem.getTempRoot()->makeSubDir("SampleSnapshots");
        mSnapshotPath = mTestSystem.getTempRoot()->makeSubPath("SampleSnapshots");
    }

    void snapshotDraw() {
        fprintf(stderr, "Performing snapshot draw %s\n", __func__);

        // TODO(benzene): preeeetty sure I'm duplicating stuff.
        // multiple inheritance from the same class with protected members is
        // not the way to go. Need to restructure this; perhaps make this a subclass
        // of SampleApplication defined in the same file, then write a different 'sample'
        // based on HelloTriangle, but make it separate and specifically for the tests.

        // It's too bad we can't just plug HelloTriangle in, but hey this is better
        // than it crashing / not using the right fields.

        // Maybe we can make it work with some templating?
        // Instruct this class to specifically use the template class's
        // versions of the functions?
        // idk man

        // FrameBuffer* fb = FrameBuffer::getFB();
        // if (!fb) {
        //     FAIL() << "Could not get FrameBuffer during snapshot application test.";
        // }

        // set up paths to save snapshots
        // std::string timeStamp =
        //         std::to_string(android::base::System::get()->getUnixTime())
        //         + "-" + std::to_string(framesDone);
        // std::string snapshotFile =
        //         mSnapshotPath + PATH_SEP "snapshot_" + timeStamp + ".snap";
        // std::string textureFile =
        //         mSnapshotPath + PATH_SEP "textures_" + timeStamp + ".stex";

        // // Save snapshot
        // std::unique_ptr<StdioStream> m_stream(new StdioStream(
        //         fopen(snapshotFile.c_str(), "wb"), StdioStream::kOwner));
        // auto a_stream = static_cast<android::base::Stream*>(m_stream.get());
        // std::shared_ptr<TextureSaver> m_texture_saver(new TextureSaver(StdioStream(
        //         fopen(textureFile.c_str(), "wb"), StdioStream::kOwner)));

        fprintf(stderr, "Saving %s\n", __func__);
        //mFb->onSave(a_stream, m_texture_saver);
        fprintf(stderr, "Done saving %s\n", __func__);

        mDrawFunc();
        framesDone++;

        fprintf(stderr, "%s did a snapshot and a draw yaya\n", __func__);
    }

    void snapshotDrawLoop(uint maxFrames = 5) {
        // fprintf(stderr, "Entered snapshot draw loop %s\n", __func__);
        // this->initialize();

        // Vsync vsync(mRefreshRate);

        // while (true) {
        //     fprintf(stderr, "Starting frame %d\n", framesDone);

        //     this->snapshotDraw();
        //     //snapshotDraw();

        //     fprintf(stderr, "Done with snapshot draw\n");
        //     mFb->flushWindowSurfaceColorBuffer(mSurface);
        //     fprintf(stderr, "Done with flush collor buffer\n");
        //     vsync.waitUntilNextVsync();
        //     fprintf(stderr, "Done with vsync wait\n");
        //     if (mUseSubWindow) {
        //         mFb->post(mColorBuffer);
        //         fprintf(stderr, "Done with post colorbuffer\n");
        //         mWindow->messageLoop();
        //     }
        //     fprintf(stderr, "Done with loop\n");

        //     if (framesDone >= maxFrames) {
        //         // mWindow->destroy();
        //         // mFb->finalize();
        //         // fprintf(stderr, "Destroying window %s\n", __func__);
        //         return;
        //     }
        // }
    }

private:
    uint framesDone = 0;
    std::function<void()> mDrawFunc;

    android::base::TestSystem mTestSystem;
    std::string mSnapshotPath = {};

};

}  // namespace emugl
