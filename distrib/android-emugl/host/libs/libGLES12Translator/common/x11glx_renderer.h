#include "common/rendering_interface.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

namespace emugl {

struct glxContextGPU : ContextGPU {
    GLXDrawable surf;
    GLXContext glxc;
    glxContextGPU(GLXContext c, GLXDrawable s) {
        surf = s;
        glxc = c;
    }
    // void* ToNative() { return s; } // never used
    void Lock() { return; } // NYI
    void Unlock() { return; } // NYI
};

class X11Renderer : public RendererInterface {
    private:
        Display                 *x11_dpy;
        Window                  x11_root;
        XVisualInfo             *x11_vi;
        Colormap                x11_cmap;
        XSetWindowAttributes    x11_swa;
        Window                  x11_win;
        XWindowAttributes       x11_gwa;
        XEvent                  x11_xev;
        glxContextGPU* bound_cxt;
        
        int x11_params_width;
        int x11_params_height;
        int x11_params_display_density;
        int x11_params_vsync_period;
        

    public:
        X11Renderer();

        virtual void GetRenderParams(RenderParams* params) const;

        // virtual void AddGraphicsObserver(GraphicsObserver* observer);
        // virtual void RemoveGraphicsObserver(GraphicsObserver* observer);

        virtual glxContextGPU* CreateContext(const Attributes& attribs, glxContextGPU* shared_context);

        virtual bool BindContext(glxContextGPU* context);

        virtual bool SwapBuffers(glxContextGPU* context);

        virtual void DestroyContext(glxContextGPU* context);
};

}


