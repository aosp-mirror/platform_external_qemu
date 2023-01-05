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

#include "OpenglRender/RenderLib.h"

#include "aemu/base/Compiler.h"
#include "host-common/vm_operations.h"
#include "host-common/misc.h"

#include <memory>

namespace emugl {

class RenderLibImpl final : public RenderLib {
public:
    RenderLibImpl() = default;

    virtual void setRenderer(SelectedRenderer renderer) override;
    virtual void setAvdInfo(bool phone, int api) override;
    virtual void getGlesVersion(int* maj, int* min) override;
    virtual void setLogger(emugl_logger_struct logger) override;
    virtual void setGLObjectCounter(
            android::base::GLObjectCounter* counter) override;
    virtual void setCrashReporter(emugl_crash_reporter_t reporter) override;
    virtual void setFeatureController(emugl_feature_is_enabled_t featureController) override;
    virtual void setSyncDevice(emugl_sync_create_timeline_t,
                               emugl_sync_create_fence_t,
                               emugl_sync_timeline_inc_t,
                               emugl_sync_destroy_timeline_t,
                               emugl_sync_register_trigger_wait_t,
                               emugl_sync_device_exists_t) override;

    virtual void setDmaOps(emugl_dma_ops) override;

    virtual void setVmOps(const QAndroidVmOperations &vm_operations) override;
    virtual void setAddressSpaceDeviceControlOps(struct address_space_device_control_ops* ops) override;

    virtual void setWindowOps(const QAndroidEmulatorWindowAgent &window_operations,
                              const QAndroidMultiDisplayAgent &multi_display_operations) override;

    virtual void setUsageTracker(android::base::CpuUsage* cpuUsage,
                                 android::base::MemoryTracker* memUsage) override;
    virtual void setGrallocImplementation(
            GrallocImplementation gralloc) override;

    virtual void* getGLESv2Dispatch(void) override;

    virtual void* getEGLDispatch(void) override;

    virtual bool getOpt(RenderOpt* opt) override;

    virtual RendererPtr initRenderer(int width,
                                     int height,
                                     bool useSubWindow,
                                     bool egl2egl) override;

    OnLastColorBufferRef getOnLastColorBufferRef() override;

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(RenderLibImpl);

private:
    std::weak_ptr<Renderer> mRenderer;
};

}  // namespace emugl
