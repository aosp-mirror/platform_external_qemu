#pragma once

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Optional.h"
#include "android/base/Result.h"
#include "android/base/memory/LazyInstance.h"
#include "android/utils/compiler.h"
#include "android/videoinjection/VideoInjectionController.h"


#include <memory>
#include <vector>

namespace android {
namespace videoplayback {

class DefaultFrameRendererImpl {
public:
    DefaultFrameRendererImpl(const GLESv2Dispatch* gles2,
                             int width,
                             int height);
    ~DefaultFrameRendererImpl() = default;

    bool initialize();
    void render();

private:
    const GLESv2Dispatch* const mGles2;
    const int mRenderWidth;
    const int mRenderHeight;
    bool displayDefault;
};

}  // namespace videoplayback
}  // namespace android
