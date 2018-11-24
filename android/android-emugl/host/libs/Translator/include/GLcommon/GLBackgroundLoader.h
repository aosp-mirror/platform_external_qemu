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

#include "android/snapshot/TextureLoader.h"
#include "emugl/common/thread.h"
#include "GLcommon/TranslatorIfaces.h"

#include <EGL/egl.h>

#include <atomic>
#include <memory>

class GLBackgroundLoader : public emugl::InterruptibleThread {
public:
    GLBackgroundLoader(const android::snapshot::ITextureLoaderWPtr& textureLoaderWeak,
                       const EGLiface& eglIface,
                       const GLESiface& glesIface,
                       SaveableTextureMap& textureMap) :
        m_textureLoaderWPtr(textureLoaderWeak),
        m_eglIface(eglIface),
        m_glesIface(glesIface),
        m_textureMap(textureMap) { }
    ~GLBackgroundLoader() {
        wait(nullptr);
        m_textureMap.clear();
    }

    intptr_t main();
    bool wait(intptr_t* exitStatus) override;
    void interrupt() override;

private:
    std::atomic<int> m_loadDelayMs { 10 };
    std::atomic<bool> m_interrupted { false };

    const android::snapshot::ITextureLoaderWPtr m_textureLoaderWPtr;
    const EGLiface& m_eglIface;
    const GLESiface& m_glesIface;

    SaveableTextureMap& m_textureMap;
};
