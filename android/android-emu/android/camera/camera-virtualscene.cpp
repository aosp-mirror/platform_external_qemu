/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/camera/camera-virtualscene.h"

#include "android/base/memory/LazyInstance.h"
#include "android/camera/camera-format-converters.h"
#include "android/virtualscene/VirtualSceneRenderer.h"
#include "emugl/common/OpenGLDispatchLoader.h"

#include <vector>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(camera)

#define VIRTUALSCENE_PIXEL_FORMAT V4L2_PIX_FMT_RGB32

/*******************************************************************************
 *                     CameraDevice routines
 ******************************************************************************/

/*
 * Describes a connection to an actual camera device.
 */
class VirtualSceneCameraDevice {
public:
    VirtualSceneCameraDevice();
    ~VirtualSceneCameraDevice();

    CameraDevice* getCameraDevice() { return &header_; }

    int startCapturing(uint32_t pixel_format,
                       int frame_width,
                       int frame_height);
    void stopCapturing();

    int readFrame(ClientFrameBuffer* framebuffers,
                  int fbs_num,
                  float r_scale,
                  float g_scale,
                  float b_scale,
                  float exp_comp);

private:
    // Initialize EGL, returns false on failure.
    bool initializeEgl();

    // Set and unset the EGL context on the current thread.
    bool setEglCurrent();
    void unsetEglCurrent();

    /* Common header. */
    CameraDevice header_;

    std::vector<uint8_t> framebuffer_data_;

    int framebuffer_width_ = 0;
    int framebuffer_height_ = 0;

    GLuint gl_fbo_ = 0;
    GLuint gl_fbo_texture_ = 0;

    // Dispatch tables for EGL and GLESv2 APIs. Note that these will be nullptr
    // if there was a problem when loading the host libraries.
    const EGLDispatch* egl_dispatch_ = nullptr;
    const GLESv2Dispatch* gles2_ = nullptr;

    EGLDisplay egl_display_ = EGL_NO_DISPLAY;
    EGLContext egl_context_ = EGL_NO_CONTEXT;
    EGLSurface egl_surface_ = EGL_NO_SURFACE;
    bool egl_initialized_ = false;
};

VirtualSceneCameraDevice::VirtualSceneCameraDevice() {
    header_.opaque = this;
}

VirtualSceneCameraDevice::~VirtualSceneCameraDevice() {
    stopCapturing();
}

int VirtualSceneCameraDevice::startCapturing(uint32_t pixel_format,
                                             int frame_width,
                                             int frame_height) {
    D("%s: Start capturing at %d x %d", __FUNCTION__, frame_width,
      frame_height);

    if (pixel_format != VIRTUALSCENE_PIXEL_FORMAT) {
        const char* fourcc = reinterpret_cast<const char*>(&pixel_format);
        E("%s: VirtualSceneCamera does not support pixel format '%c%c%c%c'",
          __FUNCTION__, fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        return -1;
    }

    framebuffer_data_.resize(4 * frame_width * frame_height);

    framebuffer_width_ = frame_width;
    framebuffer_height_ = frame_height;

    if (!initializeEgl() || !setEglCurrent()) {
        E("%s: initializeEgl failed", __FUNCTION__);
        stopCapturing();
        return -1;
    }

    egl_initialized_ = true;

    VirtualSceneRenderer::initialize(gles2_);

    unsetEglCurrent();
    return 0;
}

/* Resets camera device after capturing.
 * Since new capture request may require different frame dimensions we must
 * reset camera device by reopening its handle. Otherwise attempts to set up new
 * frame properties (different from the previous one) may fail. */
void VirtualSceneCameraDevice::stopCapturing() {
    if (egl_initialized_) {
        // Only call setEglCurrent if egl is initialized, to avoid an infinite
        // loop with setEglCurrent.
        if (!setEglCurrent()) {
            return;
        }

        VirtualSceneRenderer::uninitialize();
    }

    if (egl_dispatch_ && egl_display_ != EGL_NO_DISPLAY) {
        unsetEglCurrent();

        egl_dispatch_->eglDestroySurface(egl_display_, egl_surface_);
        egl_dispatch_->eglDestroyContext(egl_display_, egl_context_);
        // Don't eglTerminate the display, we don't own the instance.
        egl_display_ = EGL_NO_DISPLAY;
    }

    framebuffer_data_.clear();
    framebuffer_width_ = 0;
    framebuffer_height_ = 0;

    egl_initialized_ = false;
}

int VirtualSceneCameraDevice::readFrame(ClientFrameBuffer* framebuffers,
                                        int fbs_num,
                                        float r_scale,
                                        float g_scale,
                                        float b_scale,
                                        float exp_comp) {
    if (!setEglCurrent()) {
        return -1;
    }

    gles2_->glViewport(0, 0, framebuffer_width_, framebuffer_height_);

    VirtualSceneRenderer::render();
    egl_dispatch_->eglSwapBuffers(egl_display_, egl_surface_);
    gles2_->glReadPixels(0, 0, framebuffer_width_, framebuffer_height_, GL_RGBA,
                         GL_UNSIGNED_BYTE, framebuffer_data_.data());

    /* Convert frame to the receiving buffers. */
    int result = convert_frame(
            framebuffer_data_.data(), VIRTUALSCENE_PIXEL_FORMAT,
            framebuffer_data_.size(), framebuffer_width_, framebuffer_height_,
            framebuffers, fbs_num, r_scale, g_scale, b_scale, exp_comp);

    unsetEglCurrent();

    return result;
}

bool VirtualSceneCameraDevice::initializeEgl() {
    egl_dispatch_ = LazyLoadedEGLDispatch::get();
    gles2_ = LazyLoadedGLESv2Dispatch::get();
    egl_display_ = egl_dispatch_->eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (egl_display_ == EGL_NO_DISPLAY) {
        E("%s: eglGetDisplay failed, error %d", __FUNCTION__,
          egl_dispatch_->eglGetError());
        return false;
    }

    EGLint egl_maj = 0;
    EGLint egl_min = 0;

    // Try to initialize EGL display.
    // Initializing an already-initialized display is OK.
    if (egl_dispatch_->eglInitialize(egl_display_, &egl_maj, &egl_min) ==
        EGL_FALSE) {
        E("%s: eglInitialize failed, error %d", __FUNCTION__,
          egl_dispatch_->eglGetError());
        stopCapturing();
        return -1;
    }

    // Get an EGL config.
    const EGLint attribs[] = {EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES_BIT,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_RED_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              16,
                              EGL_NONE};
    EGLint num_config = 0;
    EGLConfig egl_config;
    EGLBoolean choose_result = egl_dispatch_->eglChooseConfig(
            egl_display_, attribs, &egl_config, 1, &num_config);
    if (choose_result == EGL_FALSE || num_config < 1) {
        E("%s: eglChooseConfig failed, error %d", __FUNCTION__,
          egl_dispatch_->eglGetError());
        return false;
    }

    // Create a context.
    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    egl_context_ = egl_dispatch_->eglCreateContext(
            egl_display_, egl_config, EGL_NO_CONTEXT, context_attribs);
    if (egl_context_ == EGL_NO_CONTEXT) {
        E("%s: eglCreateContext failed, error %d", __FUNCTION__,
          egl_dispatch_->eglGetError());
        return false;
    }

    // Finally, create a window surface associated with this widget.
    static const EGLint pbuf_attribs[] = {EGL_WIDTH, framebuffer_width_,
                                          EGL_HEIGHT, framebuffer_height_,
                                          EGL_NONE};
    egl_surface_ = egl_dispatch_->eglCreatePbufferSurface(
            egl_display_, egl_config, pbuf_attribs);
    if (egl_surface_ == EGL_NO_SURFACE) {
        E("%s: eglCreatePbufferSurface failed, error %d", __FUNCTION__,
          egl_dispatch_->eglGetError());
        return false;
    }

    return true;
}

bool VirtualSceneCameraDevice::setEglCurrent() {
    const EGLBoolean result = egl_dispatch_->eglMakeCurrent(
            egl_display_, egl_surface_, egl_surface_, egl_context_);
    if (result == EGL_FALSE) {
        E("%s: eglMakeCurrent failed, error %d", __FUNCTION__,
          egl_dispatch_->eglGetError());

        // Explicitly set egl_initialized to false here to avoid an infinite
        // loop with stopCapturing.
        egl_initialized_ = false;
        stopCapturing();
        return false;
    }

    return true;
}

void VirtualSceneCameraDevice::unsetEglCurrent() {
    const EGLBoolean egl_result = egl_dispatch_->eglMakeCurrent(
            egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (egl_result == EGL_FALSE) {
        W("%s: Could not unset eglMakeCurrent error %d", __FUNCTION__,
          egl_dispatch_->eglGetError());
    }
}

/*******************************************************************************
 *                     CameraDevice API
 ******************************************************************************/

extern "C" uint32_t camera_virtualscene_preferred_format() {
    return VIRTUALSCENE_PIXEL_FORMAT;
}

extern "C" CameraDevice* camera_virtualscene_open(const char* name,
                                                  int inp_channel) {
    VirtualSceneCameraDevice* cd = new VirtualSceneCameraDevice();
    return cd->getCameraDevice();
}

extern "C" int camera_virtualscene_start_capturing(CameraDevice* ccd,
                                                   uint32_t pixel_format,
                                                   int frame_width,
                                                   int frame_height) {
    // Sanity checks.
    if (!ccd || !ccd->opaque) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    VirtualSceneCameraDevice* cd =
            reinterpret_cast<VirtualSceneCameraDevice*>(ccd->opaque);
    return cd->startCapturing(pixel_format, frame_width, frame_height);
}

extern "C" int camera_virtualscene_stop_capturing(CameraDevice* ccd) {
    // Sanity checks.
    if (!ccd || !ccd->opaque) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    VirtualSceneCameraDevice* cd =
            reinterpret_cast<VirtualSceneCameraDevice*>(ccd->opaque);
    cd->stopCapturing();

    return 0;
}

extern "C" int camera_virtualscene_read_frame(CameraDevice* ccd,
                                              ClientFrameBuffer* framebuffers,
                                              int fbs_num,
                                              float r_scale,
                                              float g_scale,
                                              float b_scale,
                                              float exp_comp) {
    // Sanity checks.
    if (!ccd || !ccd->opaque) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    VirtualSceneCameraDevice* cd =
            reinterpret_cast<VirtualSceneCameraDevice*>(ccd->opaque);
    return cd->readFrame(framebuffers, fbs_num, r_scale, g_scale, b_scale,
                         exp_comp);
}

extern "C" void camera_virtualscene_close(CameraDevice* ccd) {
    // Sanity checks.
    if (!ccd || !ccd->opaque) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return;
    }

    VirtualSceneCameraDevice* cd =
            reinterpret_cast<VirtualSceneCameraDevice*>(ccd->opaque);
    delete cd;
}
