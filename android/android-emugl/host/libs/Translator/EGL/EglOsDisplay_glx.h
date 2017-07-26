/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
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

#include <X11/Xlib.h>

class GlxDisplay : public EglOS::Display {
public:
    GlxDisplay();

    virtual ~GlxDisplay();

    virtual void queryConfigs(int renderableType,
                              EglOS::AddConfigCallback* addConfigFunc,
                              void* addConfigOpaque);

    virtual bool isValidNativeWin(EglOS::Surface* win);

    virtual bool isValidNativeWin(EGLNativeWindowType win);

    virtual bool checkWindowPixelFormatMatch(
            EGLNativeWindowType win,
            const EglOS::PixelFormat* pixelFormat,
            unsigned int* width,
            unsigned int* height);

    virtual emugl::SmartPtr<EglOS::Context> createContext(
            EGLint profileMask,
            const EglOS::PixelFormat* pixelFormat,
            EglOS::Context* sharedContext);

    virtual bool destroyContext(EglOS::Context* context);

    virtual EglOS::Surface* createPbufferSurface(
            const EglOS::PixelFormat* pixelFormat,
            const EglOS::PbufferInfo* info);

    virtual bool releasePbuffer(EglOS::Surface* pb);

    virtual bool makeCurrent(EglOS::Surface* read,
                             EglOS::Surface* draw,
                             EglOS::Context* context);

    virtual void swapBuffers(EglOS::Surface* srfc);

protected:
    ::Display* mDisplay = nullptr;
};
