#include "common/rendering_interface.h"

#include <windows.h>
#include <GL/gl.h>

namespace emugl {

struct wglContextGPU : ContextGPU {
    HGLRC glc;
    wglContextGPU(HGLRC c) {
        glc = c;
    }
    void Lock() { return; } // NYI
    void Unlock() { return; } // NYI
};

class WGLRenderer : RendererInterface {
    private:
        HDC dpy;
        HWND window;
        PIXELFORMATDESCRIPTOR pfd;

        int params_width;
        int params_height;
        int params_display_density;
        int params_vsync_period;
        
    public:
        WGLRenderer();

        virtual void GetRenderParams(RenderParams* params) const;

        virtual ContextGPU* CreateContext(const Attributes& attribs, ContextGPU* shared_context);

        virtual bool BindContext(ContextGPU* context);

        virtual bool SwapBuffers(ContextGPU* context);

        virtual void DestroyContext(ContextGPU* context);

        virtual void MakeCurrent(ContextGPU* context);

        virtual void ForceContextsLost();


};

}


