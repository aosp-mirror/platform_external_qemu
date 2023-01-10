/*
* Copyright (C) 2011 The Android Open Source Project
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
#ifndef _LIB_OPENGL_RENDER_THREAD_INFO_H
#define _LIB_OPENGL_RENDER_THREAD_INFO_H

#include "android/base/files/Stream.h"
#include "RenderContext.h"
#include "WindowSurface.h"
#include "GLESv1Decoder.h"
#include "GLESv2Decoder.h"
#include "renderControl_dec.h"
#include "VkDecoder.h"
#include "StalePtrRegistry.h"
#include "SyncThread.h"

#include <functional>
#include <optional>
#include <unordered_set>

typedef uint32_t HandleType;
typedef std::unordered_set<HandleType> ThreadContextSet;
typedef std::unordered_set<HandleType> WindowSurfaceSet;

// A class used to model the state of each RenderThread related
struct RenderThreadInfo {
    // Create new instance. Only call this once per thread.
    // Future callls to get() will return this instance until
    // it is destroyed.
    RenderThreadInfo();

    // Destructor.
    ~RenderThreadInfo();

    // Release handles to contexts and surfaces it holds.
    void release();

    // Return the current thread's instance, if any, or NULL.
    static RenderThreadInfo* get();

    // Loop over all active render thread infos
    static void forAllRenderThreadInfos(std::function<void(RenderThreadInfo*)>);

    // Current EGL context, draw surface and read surface.
    HandleType currContextHandleFromLoad;
    HandleType currDrawSurfHandleFromLoad;
    HandleType currReadSurfHandleFromLoad;

    HandleType trivialContext = 0;
    HandleType trivialSurface = 0;

    RenderContextPtr currContext;
    WindowSurfacePtr currDrawSurf;
    WindowSurfacePtr currReadSurf;

    // Decoder states.
    GLESv1Decoder                   m_glDec;
    GLESv2Decoder                   m_gl2Dec;
    renderControl_decoder_context_t m_rcDec;
    VkDecoder                       m_vkDec;

    // All the contexts that are created by this render thread.
    // New emulator manages contexts in guest process level,
    // m_contextSet should be deprecated. It is only kept for
    // backward compatibility reason.
    ThreadContextSet                m_contextSet;
    // all the window surfaces that are created by this render thread
    WindowSurfaceSet                m_windowSet;

    // The unique id of owner guest process of this render thread
    uint64_t                        m_puid = 0;
    std::optional<std::string>      m_processName;

    // Whether this thread was used to perform composition.
    bool m_isCompositionThread = false;

    // Functions to save / load a snapshot
    // They must be called after Framebuffer snapshot
    void onSave(android::base::Stream* stream);
    bool onLoad(android::base::Stream* stream);

    // Sometimes we can load render thread info before
    // FrameBuffer repopulates the contexts.
    void postLoadRefreshCurrentContextSurfacePtrs();
};

#endif
