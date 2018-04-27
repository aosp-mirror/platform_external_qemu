// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Abstract interface to a pepper plugin.  We are intentionally avoiding
// using anything from the pp namespace in hopes of keeping a clean
// abstraction layer between Android code and Chrome Pepper code.

#ifndef COMMON_RENDERING_INTERFACE_H_
#define COMMON_RENDERING_INTERFACE_H_

#include <sys/types.h>

#include <string>
#include <vector>

namespace emugl {

// Opaque type of GPU context pointers.
struct ContextGPU {
    virtual ~ContextGPU() = default;

    // never used
    // virtual void* ToNative() = 0;
    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};

class GraphicsObserver {
public:
    virtual void OnGraphicsResize(int width, int height) {}

    virtual void OnGraphicsContextsLost() {}
    virtual void OnGraphicsContextsRestored() {}
};

class RendererInterface {
public:
#ifndef _WIN32
    using Attributes = std::vector<int32_t>;
#else
    typedef std::vector<int> Attributes;
#endif

    struct RenderParams {
        RenderParams() {}

        // Width of display in actual pixels.
        int width{0};
        // Width of display in actual pixels.
        int height{0};
        // Device display density.
        int display_density{0};
        // Vsync period.
        int vsync_period{0};
        // Device scale from device independent pixels to actual pixels.
        float device_render_to_view_pixels{0.f};
        // Like crx_render_to_view_pixels, controls the size of the
        // Graphics3D/Image2D resource. See also common/options.h.
        // TODO: common/options.h does not apply beyond arc, figoure out what to
        // do with this
        float crx_render_to_view_pixels{0.f};

        bool operator==(const RenderParams& rhs) const {
            return width == rhs.width && height == rhs.height &&
                   device_render_to_view_pixels ==
                           rhs.device_render_to_view_pixels &&
                   crx_render_to_view_pixels == rhs.crx_render_to_view_pixels;
        }
        bool operator!=(const RenderParams& rhs) const {
            return !operator==(rhs);
        }

        int ConvertFromDIP(float xy_in_dip) const {
            float scale =
                    device_render_to_view_pixels * crx_render_to_view_pixels;
            return xy_in_dip * scale;
        }
        float ConvertToDIP(int xy) const {
            float scale =
                    device_render_to_view_pixels * crx_render_to_view_pixels;
            return static_cast<float>(xy) / scale;
        }
    };

    virtual ~RendererInterface() = default;

    virtual void GetRenderParams(RenderParams* params) const = 0;

    virtual ContextGPU* CreateContext(const Attributes& attribs,
                                      ContextGPU* shared_context) = 0;

    virtual bool BindContext(ContextGPU* context) = 0;

    virtual bool SwapBuffers(ContextGPU* context) = 0;

    virtual void DestroyContext(ContextGPU* context) = 0;

    // Needed for non-pepper3d implementations
    virtual void MakeCurrent(ContextGPU* context) = 0;

    // Used for test purpose from org.chromium.arc.Debug
    virtual void ForceContextsLost() = 0;
};

}  // namespace emugl

#endif  // COMMON_RENDERING_INTERFACE_H_
