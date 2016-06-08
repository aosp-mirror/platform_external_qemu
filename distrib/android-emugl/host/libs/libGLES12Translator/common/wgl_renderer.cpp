#include "common/wgl_renderer.h"

#include <windows.h>
#include <GL/gl.h>

#include <cstdlib>

void swp(HDC d){
    SwapBuffers(d);
}

namespace emugl {
    WGLRenderer::WGLRenderer() {
        params_width = 1080;
        params_height = 1920;
        params_display_density = 300;
        params_vsync_period = 60;
#if 0
        // TODO: lots of windows hWindow initialization
        GLuint PixelFormat;
        WNDCLASS wc;
        DWORD dwExStyle;
        DWORD dwStlye;
        RECT WindowRect;

        // BEGIN tedious window creation stuff
        WindowRect.left=(long)0;
        WindowRect.right=(long)width;
        WindowRect.top=(long)0;
        WindowRect.bottom=(long)height;

        fullscreen=false;

        hInstance = GetModuleHandle(NULL);

        // Tedious window class stuff
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; /// redraw on size, own dc.
        wc.lpfnWndProc = (WNDPROC) WndProc; // ptr to wndproc to handle messages
        wc.cbClsExtra = 0; // Extra window data fields.
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance; // set instance for window class
        wc.hIcon = LoadIcon(NULL, IDI_WINLOGO); // Load default icon
        wc.hCursor = LoadCursor(NULL, IDC_ARROW); // load arrow pointer
        wc.hbrBackground = NULL; // No bg required for opengl (vs. blackbrush)
        wc.lpszMenuName = NULL; // We don't want a menu
        wc.lpszClassName = "OpenGL"; // Set the class name

    if(!RegisterClass(&wc)) {
        MessageBox(NULL, "Failed to register the window class.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
        return;
    }

        dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle=WS_OVERLAPPEDWINDOW;

    // Adjust window to true requested size.
    AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

    // create the window (finally!)
    if(!(window=CreateWindowEx(dwExStyle, "OpenGL", title, 
                               dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                               0,0,
                               WindowRect.right - WindowRect.left,
                               WindowRect.bottom - WindowRect.top,
                               NULL, // NO parent window
                               NULL, // NO menu
                               hInstance,
                               NULL))) {
        MessageBox(NULL, "Window creation error.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
        return ;
    }

        // end tedious window creation stuff
        // below is more EGL-like stuff

    static PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), //size 
        1, // version
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // dwFlags
        PFD_TYPE_RGBA, // Pixeltype
        24, // colorbits
        0,0, // red bits/shift
        0,0, // green bits/shift
        0,0,  // blue bits/shift
        0, // no alpha buffer
        0, // no shift bit
        0, // no acc buffer
        0,0,0,0, // acc bits ignored
        16, // 16 bit stencil buffer
        0, // no stencil buffer
        0, // no aux  buffer
        PFD_MAIN_PLANE, // main drawing layer
        0, // reserved
        0,0,0 // layer masks ignored
    };

        dpy = ::GetDC(window);
        if(!dpy) {
            MessageBox(NULL, "Can't get device context from window.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
        }

        GLuint PixelFormat;
        if(!(PixelFormat=ChoosePixelFormat(dpy,&pfd))) {
            MessageBox(NULL, "Can't find a suitable pixel format.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
        }

        if(!(SetPixelFormat(dpy,PixelFormat, &pfd))) {
            MessageBox(NULL, "Can't find a suitable pixel format.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
        }
        // and then if we were doing typical WGL, we'd create a context. But, since this is abstract renderer, we probably need to stop here.
        // A question is how the config is going to work with this.
       #endif
    }

void WGLRenderer::GetRenderParams(RenderParams* params) const {
    params->width = params_width;
    params->height = params_height;
    params->display_density = params_display_density;
    params->vsync_period = params_vsync_period;
    params->device_render_to_view_pixels = 0.f;
    params->crx_render_to_view_pixels = 0.f;
}

ContextGPU* WGLRenderer::CreateContext(const Attributes& attribs, ContextGPU* shared_context) {
    wglContextGPU* res = new wglContextGPU(NULL);
    HGLRC new_cxt;
    if(!(new_cxt=wglCreateContext(dpy))) {
        MessageBox(NULL, "Can't create a GL rendering context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return NULL;
    }
    res->glc = new_cxt;
    return (ContextGPU*)res;
}

bool WGLRenderer::BindContext(ContextGPU* context) {
    wglContextGPU* wglc = (wglContextGPU*)context;
    return true;
}

bool WGLRenderer::SwapBuffers(ContextGPU* context) {
    wglContextGPU* wglc = (wglContextGPU*)context;
    return ::SwapBuffers(dpy) == TRUE;
}

void WGLRenderer::DestroyContext(ContextGPU* context) {
    wglContextGPU* wglc = (wglContextGPU*)context;
    return;
}

void WGLRenderer::ForceContextsLost() {
    return;
}

void WGLRenderer::MakeCurrent(ContextGPU* cxt) { return; }


} // namespace emugl
