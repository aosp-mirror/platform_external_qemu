#include "android/camera/camera-virtualscene-utils.h"

#define VIRTUALSCENE_PIXEL_FORMAT V4L2_PIX_FMT_RGB32

namespace android {
namespace virtualscene {

VirtualSceneCameraDevice::VirtualSceneCameraDevice(
        std::unique_ptr<Renderer> renderer)
    : renderer(std::move(renderer)) {
    mHeader.opaque = this;
}

VirtualSceneCameraDevice::~VirtualSceneCameraDevice() {
    stopCapturing();
}

int VirtualSceneCameraDevice::startCapturing(uint32_t pixelFormat,
                                             int frameWidth,
                                             int frameHeight) {
    D("%s: Start capturing at %d x %d", __FUNCTION__, frameWidth, frameHeight);

    mFramebufferData.resize(4 * frameWidth * frameHeight);

    mFramebufferWidth = frameWidth;
    mFramebufferHeight = frameHeight;

    bool succeeded = false;
    if (initializeEgl()) {
        auto context = makeEglCurrent();
        if (context.isValid()) {
            if (renderer->initialize(mGles2, frameWidth, frameHeight)) {
                succeeded = true;
            } else {
                E("%s: Renderer initialize failed", __FUNCTION__);
            }
        }
    }

    if (!succeeded) {
        stopCapturing();
        return -1;
    }

    return 0;
}

// Resets camera device after capturing.
// Since new capture request may require different frame dimensions we must
// reset camera device by reopening its handle. Otherwise attempts to set up new
// frame properties (different from the previous one) may fail.
void VirtualSceneCameraDevice::stopCapturing() {
    if (mEglInitialized) {
        // Only call makeEglCurrent if egl is initialized, to avoid an infinite
        // loop when eglMakeCurrent fails.
        auto context = makeEglCurrent();
        if (context.isValid()) {
            renderer->uninitialize();
        }
    }

    if (mEglDispatch && mEglDisplay != EGL_NO_DISPLAY) {
        mEglDispatch->eglDestroySurface(mEglDisplay, mEglSurface);
        mEglDispatch->eglDestroyContext(mEglDisplay, mEglContext);
        // Don't eglTerminate the display, we don't own the instance.
        mEglDisplay = EGL_NO_DISPLAY;
    }

    mFramebufferData.clear();
    mFramebufferData.shrink_to_fit();
    mFramebufferWidth = 0;
    mFramebufferHeight = 0;

    mEglInitialized = false;
}

int VirtualSceneCameraDevice::readFrame(ClientFrame* resultFrame,
                                        float rScale,
                                        float gScale,
                                        float bScale,
                                        float expComp) {
    auto context = makeEglCurrent();
    if (!context.isValid()) {
        return -1;
    }

    resultFrame->frame_time = renderer->render();
    mEglDispatch->eglSwapBuffers(mEglDisplay, mEglSurface);
    mGles2->glReadPixels(0, 0, mFramebufferWidth, mFramebufferHeight, GL_RGBA,
                         GL_UNSIGNED_BYTE, mFramebufferData.data());

    // Convert frame to the receiving buffers.
    return convert_frame(mFramebufferData.data(), VIRTUALSCENE_PIXEL_FORMAT,
                         mFramebufferData.size(), mFramebufferWidth,
                         mFramebufferHeight, resultFrame, rScale, gScale,
                         bScale, expComp);
}

bool VirtualSceneCameraDevice::initializeEgl() {
    mEglDispatch = emugl::LazyLoadedEGLDispatch::get();
    mGles2 = emugl::LazyLoadedGLESv2Dispatch::get();
    mEglDisplay = mEglDispatch->eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (mEglDisplay == EGL_NO_DISPLAY) {
        E("%s: eglGetDisplay failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    EGLint eglMaj = 0;
    EGLint eglMin = 0;

    // Try to initialize EGL display.
    // Initializing an already-initialized display is OK.
    if (mEglDispatch->eglInitialize(mEglDisplay, &eglMaj, &eglMin) ==
        EGL_FALSE) {
        E("%s: eglInitialize failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
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
    EGLint numConfig = 0;
    EGLConfig eglConfig;
    EGLBoolean chooseResult = mEglDispatch->eglChooseConfig(
            mEglDisplay, attribs, &eglConfig, 1, &numConfig);
    if (chooseResult == EGL_FALSE || numConfig < 1) {
        E("%s: eglChooseConfig failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    // Create a context.
    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    mEglContext = mEglDispatch->eglCreateContext(
            mEglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (mEglContext == EGL_NO_CONTEXT) {
        E("%s: eglCreateContext failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    // Finally, create a window surface associated with this widget.
    const EGLint pbufferAttribs[] = {EGL_WIDTH, mFramebufferWidth, EGL_HEIGHT,
                                     mFramebufferHeight, EGL_NONE};
    mEglSurface = mEglDispatch->eglCreatePbufferSurface(mEglDisplay, eglConfig,
                                                        pbufferAttribs);
    if (mEglSurface == EGL_NO_SURFACE) {
        E("%s: eglCreatePbufferSurface failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    mEglInitialized = true;
    return true;
}

ScopedEglContext VirtualSceneCameraDevice::makeEglCurrent() {
    const EGLBoolean result = mEglDispatch->eglMakeCurrent(
            mEglDisplay, mEglSurface, mEglSurface, mEglContext);
    if (result == EGL_FALSE) {
        E("%s: eglMakeCurrent failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());

        // Explicitly set mEglInitialized to false here to avoid an infinite
        // loop with stopCapturing.
        mEglInitialized = false;
        stopCapturing();

        // Return an empty ScopedEglContext because we failed to call
        // eglMakeCurrent.
        return ScopedEglContext(nullptr, EGL_NO_DISPLAY);
    }

    return ScopedEglContext(mEglDispatch, mEglDisplay);
}


}  // namespace virtualscene
}  // namespace android
