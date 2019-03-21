// Copyright (C) 2016 The Android Open Source Project
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

#include <memory>

#include "OpenglRender/Renderer.h"
#include "OpenglRender/render_api_types.h"
#include "android/base/files/Stream.h"
#include "android/emulation/RefcountPipe.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/opengl/emugl_config.h"

namespace android {
namespace base {

class CpuUsage;
class MemoryTracker;
class GLObjectCounter;

} // namespace base
} // namespace android

namespace emugl {

struct RenderOpt {
    void* display;
    void* surface;
    void* config;
};

using android::emulation::OnLastColorBufferRef;

// RenderLib - root interface for the GPU emulation library
//  Use it to set the library-wide parameters (logging, crash reporting) and
//  create indivudual renderers that take care of drawing windows.
class RenderLib {
public:
    virtual ~RenderLib() = default;

    // Get the selected renderer
    virtual void setRenderer(SelectedRenderer renderer) = 0;
    // Tell emugl the API version of the system image
    virtual void setAvdInfo(bool phone, int api) = 0;
    // Get the GLES major/minor version determined by libOpenglRender.
    virtual void getGlesVersion(int* maj, int* min) = 0;
    virtual void setLogger(emugl_logger_struct logger) = 0;
    virtual void setGLObjectCounter(
            android::base::GLObjectCounter* counter) = 0;
    virtual void setCrashReporter(emugl_crash_reporter_t reporter) = 0;
    virtual void setFeatureController(emugl_feature_is_enabled_t featureController) = 0;
    virtual void setSyncDevice(emugl_sync_create_timeline_t,
                               emugl_sync_create_fence_t,
                               emugl_sync_timeline_inc_t,
                               emugl_sync_destroy_timeline_t,
                               emugl_sync_register_trigger_wait_t,
                               emugl_sync_device_exists_t) = 0;

    // Sets the function use to read from the guest
    // physically contiguous DMA region at particular offsets.
    virtual void setDmaOps(emugl_dma_ops) = 0;

    virtual void setVmOps(const QAndroidVmOperations &vm_operations) = 0;

    virtual void setWindowOps(const QAndroidEmulatorWindowAgent &window_operations) = 0;

    virtual void setUsageTracker(android::base::CpuUsage* cpuUsage,
                                 android::base::MemoryTracker* memUsage) = 0;

    virtual void* getGL(void) = 0;

    virtual void* getEGL(void) = 0;

    virtual bool getOpt(RenderOpt* opt) = 0;

    // initRenderer - initialize the OpenGL renderer object.
    //
    // |width| and |height| are the framebuffer dimensions that will be reported
    // to the guest display driver.
    //
    // |useSubWindow| is true to indicate that renderer has to support an
    // OpenGL subwindow. If false, it only supports setPostCallback().
    // See Renderer.h for more info.
    //
    // There might be only one renderer.
    virtual RendererPtr initRenderer(int width, int height,
                                     bool useSubWindow, bool egl2egl) = 0;

    virtual OnLastColorBufferRef getOnLastColorBufferRef() = 0;
};

using RenderLibPtr = std::unique_ptr<RenderLib>;

}  // namespace emugl
