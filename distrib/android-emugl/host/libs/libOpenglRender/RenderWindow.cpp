// Copyright 2014 The Android Open Source Project
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

#include "RenderWindow.h"

#include "FrameBuffer.h"

// TODO(digit): This implementation just proxies the method calls directly
//              to the FrameBuffer class. In the future, everything will be
//              moved to a dedicated thread to ensure there is no conflict
//              between the display/context of the main window and the one
//              from the sub-window.

RenderWindow::RenderWindow(int width, int height) :
        mValid(false),
        mHasSubWindow(false) {
    mValid = FrameBuffer::initialize(width, height);
}

RenderWindow::~RenderWindow() {
    removeSubWindow();
    FrameBuffer::finalize();
}

bool RenderWindow::getHardwareStrings(const char** vendor,
                                      const char** renderer,
                                      const char** version) {
    FrameBuffer* fb = FrameBuffer::getFB();
    if (!fb) {
        return false;
    }
    fb->getGLStrings(vendor, renderer, version);
    return true;
}

void RenderWindow::setPostCallback(OnPostFn onPost, void* onPostContext) {
    FrameBuffer* fb = FrameBuffer::getFB();
    fb->setPostCallback(onPost, onPostContext);
}

bool RenderWindow::setupSubWindow(FBNativeWindowType window,
                                  int x,
                                  int y,
                                  int width,
                                  int height,
                                  float zRot) {
    if (mHasSubWindow) {
        return false;
    }
    mHasSubWindow = FrameBuffer::setupSubWindow(
            window, x, y, width, height, zRot);

    return mHasSubWindow;
}

bool RenderWindow::removeSubWindow() {
    if (!mHasSubWindow) {
        return false;
    }
    mHasSubWindow = false;
    return FrameBuffer::removeSubWindow();
}

void RenderWindow::setRotation(float zRot) {
    FrameBuffer* fb = FrameBuffer::getFB();
    if (fb) {
        fb->setDisplayRotation(zRot);
    }
}

void RenderWindow::repaint() {
    FrameBuffer* fb = FrameBuffer::getFB();
    if (fb) {
        fb->repost();
    }
}
