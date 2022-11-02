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
#include "RenderLibImpl.h"

#include "FrameBuffer.h"
#include "RendererImpl.h"

#include "aemu/base/CpuUsage.h"
#include "aemu/base/files/Stream.h"
#include "emugl/common/address_space_device_control_ops.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/dma_device.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/logging.h"
#include "emugl/common/misc.h"
#include "emugl/common/sync_device.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/DispatchTables.h"

namespace emugl {

void RenderLibImpl::setRenderer(SelectedRenderer renderer) {
    emugl::setRenderer(renderer);
}

void RenderLibImpl::setAvdInfo(bool phone, int api) {
    emugl::setAvdInfo(phone, api);
}

void RenderLibImpl::getGlesVersion(int* maj, int* min) {
    emugl::getGlesVersion(maj, min);
}

void RenderLibImpl::setLogger(emugl_logger_struct logger) {
    set_emugl_logger(logger.coarse);
    set_emugl_cxt_logger(logger.fine);
}

void RenderLibImpl::setGLObjectCounter(
        android::base::GLObjectCounter* counter) {
    emugl::setGLObjectCounter(counter);
}

void RenderLibImpl::setCrashReporter(emugl_crash_reporter_t reporter) {
    set_emugl_crash_reporter(reporter);
}

void RenderLibImpl::setFeatureController(emugl_feature_is_enabled_t featureController) {
    set_emugl_feature_is_enabled(featureController);
}

void RenderLibImpl::setSyncDevice
    (emugl_sync_create_timeline_t create_timeline,
     emugl_sync_create_fence_t create_fence,
     emugl_sync_timeline_inc_t timeline_inc,
     emugl_sync_destroy_timeline_t destroy_timeline,
     emugl_sync_register_trigger_wait_t register_trigger_wait,
     emugl_sync_device_exists_t device_exists) {
    set_emugl_sync_create_timeline(create_timeline);
    set_emugl_sync_create_fence(create_fence);
    set_emugl_sync_timeline_inc(timeline_inc);
    set_emugl_sync_destroy_timeline(destroy_timeline);
    set_emugl_sync_register_trigger_wait(register_trigger_wait);
    set_emugl_sync_device_exists(device_exists);
}

void RenderLibImpl::setDmaOps(emugl_dma_ops ops) {
    set_emugl_dma_get_host_addr(ops.get_host_addr);
    set_emugl_dma_unlock(ops.unlock);
}

void RenderLibImpl::setVmOps(const QAndroidVmOperations &vm_operations) {
    set_emugl_vm_operations(vm_operations);
}

void RenderLibImpl::setAddressSpaceDeviceControlOps(struct address_space_device_control_ops* ops) {
    set_emugl_address_space_device_control_ops(ops);
}

void RenderLibImpl::setWindowOps(const QAndroidEmulatorWindowAgent &window_operations,
                                 const QAndroidMultiDisplayAgent &multi_display_operations) {
    set_emugl_window_operations(window_operations);
    set_emugl_multi_display_operations(multi_display_operations);
}

void RenderLibImpl::setUsageTracker(android::base::CpuUsage* cpuUsage,
                                    android::base::MemoryTracker* memUsage) {
    emugl::setCpuUsage(cpuUsage);
    emugl::setMemoryTracker(memUsage);
}

void RenderLibImpl::setGrallocImplementation(GrallocImplementation gralloc) {
    emugl::setGrallocImplementation(gralloc);
}

void* RenderLibImpl::getGLESv2Dispatch(void) {
    return &s_gles2;
}

void* RenderLibImpl::getEGLDispatch(void) {
    return &s_egl;
}

bool RenderLibImpl::getOpt(RenderOpt* opt) {
    FrameBuffer* fb  = FrameBuffer::getFB();
    if (fb == nullptr) {
        return false;
    }

    opt->display = fb->getDisplay();
    opt->surface = fb->getWindowSurface();
    opt->config = fb->getConfig();

    return (opt->display && opt->surface  && opt->config);
}

RendererPtr RenderLibImpl::initRenderer(int width, int height,
                                        bool useSubWindow, bool egl2egl) {
    if (!mRenderer.expired()) {
        return nullptr;
    }

    const auto res = std::make_shared<RendererImpl>();
    if (!res->initialize(width, height, useSubWindow, egl2egl)) {
        return nullptr;
    }
    mRenderer = res;
    return res;
}

static void impl_onLastCbRef(uint32_t handle) {
    FrameBuffer* fb = FrameBuffer::getFB();
    if (fb)
        fb->onLastColorBufferRef(handle);
}

OnLastColorBufferRef RenderLibImpl::getOnLastColorBufferRef() {
    return (OnLastColorBufferRef)impl_onLastCbRef;
}

}  // namespace emugl
