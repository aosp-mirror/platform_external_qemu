#pragma once

#include <EGL/egl.h>

#include <memory>
class OSWindow;
class BaseWindow;

class MultiWindow {
public:
    MultiWindow(int windowWidth, int windowHeight);

private:
    std::unique_ptr<BaseWindow> mBaseWindow;
    EGLNativeWindowType mWindow = {};
    EGLContext mContext;
    EGLSurface mSurface;
    EGLDisplay mDisplay;
    int mWidth;
    int mHeight;

};
