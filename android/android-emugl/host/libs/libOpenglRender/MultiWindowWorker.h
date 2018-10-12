#pragma once

#include <EGL/egl.h>
#include <memory>

class FrameBuffer;
class ColorBuffer;
class BaseWindow;

class MultiWindowWorker {
public:
    MultiWindowWorker();
    MultiWindowWorker(int width, int height);
    ~MultiWindowWorker();

    void initMultiWindow(int width, int height);
    void post(ColorBuffer* cb);

private:
    std::unique_ptr<BaseWindow> mBaseWindow;
    EGLNativeWindowType mWindow = {};
    EGLContext mContext;
    EGLSurface mSurface;
    EGLDisplay mDisplay;
    FrameBuffer* mFb;
};
