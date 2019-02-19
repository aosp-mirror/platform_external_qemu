#include "android/camera/camera-videoplayback-default-renderer.h"

#include "android/base/Log.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/offworld/proto/offworld.pb.h"

namespace android {
namespace videoplayback {

DefaultFrameRendererImpl::DefaultFrameRendererImpl(const GLESv2Dispatch* gles2,
                                                   int width,
                                                   int height)
    : mGles2(gles2),
      mRenderWidth(width),
      mRenderHeight(width),
      displayDefault(false) {}

bool DefaultFrameRendererImpl::initialize() {
    return true;
}

void DefaultFrameRendererImpl::render() {
    if (videoinjection::VideoInjectionController::tryGetNextRequest(
                base::Ok())) {
        displayDefault = true;
    }
    mGles2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mGles2->glViewport(0, 0, mRenderWidth, mRenderHeight);

    if (displayDefault) {
        mGles2->glClearColor(1.0f, 0.6f, 0.0f, 1.0f);
    } else {
        mGles2->glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    }
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mGles2->glFrontFace(GL_CW);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mGles2->glBindTexture(GL_TEXTURE_2D, 0);
    mGles2->glUseProgram(0);
}

}  // namespace videoplayback
}  // namespace android
