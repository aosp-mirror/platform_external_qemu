// license
//
//


#include "MultiWindow.h"

#include "android/base/system/System.h"
#include "NativeSubWindow.h"
#include "standalone_common/angle-util/OSWindow.h"
#include "../../../shared/emugl/common/OpenGLDispatchLoader.h"

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
        if (!window->initialize("libOpenglRender test", width, height)) {
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

MultiWindow::MultiWindow(int windowWidth, int windowHeight) :
      mWidth(windowWidth),
      mHeight(windowHeight)
{
        mBaseWindow = std::make_unique<BaseWindow>();
        mBaseWindow->initializeWithRect(0, 0, windowWidth, windowHeight);
        mWindow = ::createSubWindow((FBNativeWindowType)(uintptr_t)(mBaseWindow->window->getFramebufferNativeWindow()),
                                    0, 0, windowWidth,
                                    windowHeight, muliWindowRepaint, this);
        if (!mWindow) {
            printf("Error create multi window");
            return;
        }

        LazyLoadedEGLDispatch::get();
        LazyLoadedGLESv2Dispatch::get();

        mDisplay = s_egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (mDisplay == EGL_NO_DISPLAY) {
            printf("Error get EGL_DEFAULT_DISPLAY");
            return;
        }

        EGLint egl_maj, egl_min;
        EGLConfig egl_config;
        if (s_egl.eglInitialize(mDisplay, &egl_maj, &egl_min) ==
            EGL_FALSE) {
            printf("Failed to initialize EGL display: EGL error %d",
                 s_egl.eglGetError());
            return;
        }
        // Get an EGL config.
        const EGLint config_attribs[] = {EGL_SURFACE_TYPE,
                                         EGL_WINDOW_BIT,
                                         EGL_RENDERABLE_TYPE,
                                         EGL_OPENGL_ES2_BIT,
                                         EGL_RED_SIZE,
                                         8,
                                         EGL_GREEN_SIZE,
                                         8,
                                         EGL_BLUE_SIZE,
                                         8,
                                         EGL_DEPTH_SIZE,
                                         24};
        EGLint num_config;
        EGLBoolean choose_result = s_egl.eglChooseConfig(
                mDisplay, config_attribs, &egl_config, 1, &num_config);
        if (choose_result == EGL_FALSE || num_config < 1) {
            printf("Failed to choose EGL config: EGL error %d",
                     s_egl.eglGetError());
            return;
        }

        // Create a context.
        EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        mContext = s_egl.eglCreateContext(
            mDisplay, egl_config, EGL_NO_CONTEXT, context_attribs);
        if (mContext == EGL_NO_CONTEXT) {
            printf("Failed to create EGL context %d", s_egl.eglGetError());
            return;
        }

        // Finally, create a window surface associated with this widget.
        mSurface = s_egl.eglCreateWindowSurface(
                mDisplay, egl_config, mWindow,
                nullptr);
        if (mSurface == EGL_NO_SURFACE) {
            printf("Failed to create an EGL surface %d", s_egl.eglGetError());
            return;
        }



}


