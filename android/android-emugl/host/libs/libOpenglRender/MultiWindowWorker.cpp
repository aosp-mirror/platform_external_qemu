#include "MultiWindowWorker.h"
#include "FrameBuffer.h"

#include "android/base/system/System.h"
#include "NativeSubWindow.h"
#include "standalone_common/angle-util/OSWindow.h"
#include "../../../shared/emugl/common/OpenGLDispatchLoader.h"

extern const GLint* getGlesMaxContextAttribs();
// Class holding the persistent test window.
class BaseWindow {
public:
    BaseWindow() {
        window = CreateOSWindow();
        printf("created\n");
    }

    ~BaseWindow() {
        if (window) {
            window->destroy();
        }
    }

    // Check on initialization if windows are available.
    bool initializeWithRect(int xoffset, int yoffset, int width, int height) {
        if (!window->initialize("multiWindow test", width, height)) {
            window->destroy();
            window = nullptr;
            return false;
        }
        window->setVisible(true);
        window->setPosition(xoffset, yoffset);
        window->messageLoop();
        return true;
    }

    void resizeWithRect(int xoffset, int yoffset, int width, int height) {
        if (!window) return;

        window->setPosition(xoffset, yoffset);
        window->resize(width, height);
        window->messageLoop();
    }

    OSWindow* window = nullptr;
};

static void muliWindowRepaint(void* para) {
    printf("muliWindowRepaint\n");
    return;
}

MultiWindowWorker::MultiWindowWorker() {
    mFb = FrameBuffer::getFB();
    mDisplay = mFb->getDisplay();
}

MultiWindowWorker::MultiWindowWorker(int width, int height) {
    mFb = FrameBuffer::getFB();
    mDisplay = mFb->getDisplay();

    initMultiWindow(width, height);
}

MultiWindowWorker::~MultiWindowWorker() {
}

void MultiWindowWorker::initMultiWindow(int width, int height) {
    mBaseWindow.reset(new BaseWindow());
    mBaseWindow->initializeWithRect(0, 0, width, height);
    mBaseWindow->resizeWithRect(100, 100, width, height);
    mWindow = ::createSubWindow((FBNativeWindowType)(uintptr_t)(mBaseWindow->window->getFramebufferNativeWindow()),
                                0, 0, width,
                                height, muliWindowRepaint, this);
    if (!mWindow) {
        printf("Error create multi window");
        return;
    }

    LazyLoadedEGLDispatch::get();
    LazyLoadedGLESv2Dispatch::get();

  // Create a context.
    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    mContext = s_egl.eglCreateContext(
        mDisplay, mFb->getConfig(), mFb->getPbufContext(), getGlesMaxContextAttribs());
    if (mContext == EGL_NO_CONTEXT) {
        printf("Failed to create EGL context %d", s_egl.eglGetError());
        return;
    }

    // Finally, create a window surface associated with this widget.
    mSurface = s_egl.eglCreateWindowSurface(
            mDisplay, mFb->getConfig(), mWindow,
            nullptr);
    if (mSurface == EGL_NO_SURFACE) {
        printf("Failed to create an EGL surface %d", s_egl.eglGetError());
        return;
    }

    s_egl.eglMakeCurrent(mDisplay, mSurface, mSurface, mContext);

}

void MultiWindowWorker::post(ColorBuffer* cb) {

    cb->postWithOverlay(cb->getTexture(), 0, 0, 0);
    s_egl.eglSwapBuffers(mDisplay, mSurface);

}
