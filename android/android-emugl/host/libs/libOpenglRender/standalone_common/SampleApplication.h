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

#include <cinttypes>
#include <functional>

class FrameBuffer;
class OSWindow;
class RenderThreadInfo;

namespace emugl {

// Determines whether the host GPU should be used.
bool shouldUseHostGpu();

// Creates or adjusts a persistent test window.
// On some systems, test window creation can fail (such as when on a headless server).
// In that case, this function will return nullptr.
OSWindow* createOrGetTestWindow(int xoffset, int yoffset, int width, int height);

// Creates a window (or runs headless) to be used in a sample app.
class SampleApplication {
public:
    SampleApplication(int windowWidth = 256, int windowHeight = 256,
                      int refreshRate = 60);
    ~SampleApplication();

    void drawLoop();

protected:
    virtual void initialize() = 0;
    virtual void draw() = 0;

private:
    int mWidth = 256;
    int mHeight = 256;
    int mRefreshRate = 60;

    bool mUseSubWindow = false;
    OSWindow* mWindow = nullptr;
    FrameBuffer* mFb = nullptr;
    RenderThreadInfo* mRenderThreadInfo = nullptr;

    int mXOffset= 400;
    int mYOffset= 400;

    unsigned int mColorBuffer = 0;
    unsigned int mContext = 0;
    unsigned int mSurface = 0;
};

} // namespace emugl
