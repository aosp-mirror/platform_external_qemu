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

#include "GLcommon/GLBackgroundLoader.h"

#include "GLcommon/GLEScontext.h"
#include "GLcommon/SaveableTexture.h"
#include "android/base/system/System.h"
#include "android/utils/system.h"

#include <EGL/eglext.h>
#include <GLES2/gl2.h>

intptr_t GLBackgroundLoader::main() {
#if SNAPSHOT_PROFILE > 1
    const auto start = get_uptime_ms();
    printf("Starting GL background loading at %" PRIu64 " ms\n", start);
#endif

    if (!m_eglIface.createAndBindAuxiliaryContext(&m_context, &m_surface)) {
        return 0;
    }

    for (const auto& it : m_textureMap) {
        if (m_interrupted) break;

        // Acquire the texture loader for each load; bail
        // in case something else happened to interrupt loading.
        auto ptr = m_textureLoaderWPtr.lock();
        if (!ptr) {
            break;
        }

        const SaveableTexturePtr& saveable = it.second;
        if (saveable) {
            m_glesIface.restoreTexture(saveable.get());
            // allow other threads to run for a while
            ptr.reset();
            android::base::System::get()->sleepMs(m_loadDelayMs);
        }
    }

    m_eglIface.unbindAndDestroyAuxiliaryContext(m_context, m_surface);
    m_textureMap.clear();

#if SNAPSHOT_PROFILE > 1
    const auto end = get_uptime_ms();
    printf("Finished GL background loading at %" PRIu64 " ms (%d ms total)\n",
           end, int(end - start));
#endif

    return 0;
}

bool GLBackgroundLoader::wait(intptr_t* exitStatus) {
    m_loadDelayMs = 0;
    return Thread::wait();
}

void GLBackgroundLoader::interrupt() {
    m_interrupted = true;
}
