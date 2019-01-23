#include "GLSnapshotTestDispatch.h"

#include "FrameBuffer.h"
#include "GLSnapshotTesting.h"
#include "GLTestUtils.h"
#include "OpenglCodecCommon/glUtils.h"
#include "RenderThreadInfo.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

namespace emugl {

using android::base::LazyInstance;
using android::base::StdioStream;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

static LazyInstance<SnapshotTestDispatch> sSnapshotTestDispatch =
        LAZY_INSTANCE_INIT;

// static
const GLESv2Dispatch* getSnapshotTestDispatch() {
    return sSnapshotTestDispatch.ptr();
}

SnapshotTestDispatch::SnapshotTestDispatch()
    : mTestSystem(PATH_SEP "progdir",
                  android::base::System::kProgramBitness,
                  PATH_SEP "homedir",
                  PATH_SEP "appdir") {
    mTestSystem.getTempRoot()->makeSubDir("SampleSnapshots");
    mSnapshotPath = mTestSystem.getTempRoot()->makeSubPath("SampleSnapshots");

    mValid = gles2_dispatch_init(this);
    if (mValid) {
        overrideFunctions();
    } else {
        fprintf(stderr, "SnapshotTestDispatch failed to initialize.\n");
        ADD_FAILURE() << "SnapshotTestDispatch could not initialize.";
    }
}

void SnapshotTestDispatch::overrideFunctions() {
    this->glDrawArrays = (glDrawArrays_t)test_glDrawArrays;
    this->glDrawElements = (glDrawElements_t)test_glDrawElements;
}

void SnapshotTestDispatch::saveSnapshot() {
    FrameBuffer* fb = FrameBuffer::getFB();
    if (!fb) {
        FAIL() << "Could not get FrameBuffer during snapshot test.";
    }

    std::string timeStamp =
            std::to_string(android::base::System::get()->getUnixTime()) + "-" +
            std::to_string(mLoadCount);
    mSnapshotFile = mSnapshotPath + PATH_SEP "snapshot_" + timeStamp + ".snap";
    mTextureFile = mSnapshotPath + PATH_SEP "textures_" + timeStamp + ".stex";
    std::unique_ptr<StdioStream> m_stream(new StdioStream(
            android_fopen(mSnapshotFile.c_str(), "wb"), StdioStream::kOwner));
    auto a_stream = static_cast<android::base::Stream*>(m_stream.get());
    std::shared_ptr<TextureSaver> m_texture_saver(new TextureSaver(StdioStream(
            android_fopen(mTextureFile.c_str(), "wb"), StdioStream::kOwner)));

    fb->onSave(a_stream, m_texture_saver);

    // Save thread's context and surface handles so we can restore the bind
    // after load is complete.
    RenderThreadInfo* threadInfo = RenderThreadInfo::get();
    if (threadInfo) {
        threadInfo->onSave(a_stream);
    }

    m_stream->close();
    m_texture_saver->done();
}

void SnapshotTestDispatch::loadSnapshot() {
    FrameBuffer* fb = FrameBuffer::getFB();
    if (!fb) {
        FAIL() << "Could not get FrameBuffer during snapshot test.";
    }

    // unbind so load will destroy previous objects
    fb->bindContext(0, 0, 0);

    std::unique_ptr<StdioStream> m_stream(new StdioStream(
            android_fopen(mSnapshotFile.c_str(), "rb"), StdioStream::kOwner));
    std::shared_ptr<TextureLoader> m_texture_loader(
            new TextureLoader(StdioStream(android_fopen(mTextureFile.c_str(), "rb"),
                                          StdioStream::kOwner)));

    fb->onLoad(m_stream.get(), m_texture_loader);

    RenderThreadInfo* threadInfo = RenderThreadInfo::get();
    if (threadInfo) {
        threadInfo->onLoad(m_stream.get());
        // rebind to context
        fb->bindContext(
                threadInfo->currContext ? threadInfo->currContext->getHndl()
                                        : 0,
                threadInfo->currDrawSurf ? threadInfo->currDrawSurf->getHndl()
                                         : 0,
                threadInfo->currReadSurf ? threadInfo->currReadSurf->getHndl()
                                         : 0);
    }

    m_stream->close();
    m_texture_loader->join();

    mLoadCount++;
}

// static
void SnapshotTestDispatch::testDraw(std::function<void()> doDraw) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    ASSERT_NE(nullptr, gl);

    FrameBuffer* fb = FrameBuffer::getFB();
    if (!fb) {
        ADD_FAILURE() << "No framebuffer, test cannot snapshot.";
        doDraw();
        return;
    }

    // save then draw
    ((SnapshotTestDispatch*)getSnapshotTestDispatch())->saveSnapshot();
    // Since current framebuffer contents are not saved, we need to draw
    // onto a clean slate in order to check the result of the draw call
    gl->glClear(GL_COLOR_BUFFER_BIT);
    doDraw();

    // save the framebuffer contents
    GLuint width, height, bytesPerPixel;
    width = fb->getWidth();
    height = fb->getHeight();
    bytesPerPixel = glUtilsPixelBitSize(GL_RGBA, GL_UNSIGNED_BYTE) / 8;
    std::vector<GLubyte> prePixels = {};
    prePixels.resize(width * height * bytesPerPixel);
    gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                     prePixels.data());

    // To verify that the snapshot is restoring our context, we modify the
    // clear color.
    std::vector<GLfloat> oldClear = {};
    oldClear.resize(4);
    gl->glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClear.data());
    EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE, oldClear));
    gl->glClearColor(1, 1, 1, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE, {1, 1, 1, 1}));

    // load and redraw
    ((SnapshotTestDispatch*)getSnapshotTestDispatch())->loadSnapshot();
    gl->glClear(GL_COLOR_BUFFER_BIT);
    doDraw();

    // check that clear is restored
    EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE, oldClear));

    // compare the framebuffer contents
    std::vector<GLubyte> postPixels = {};
    postPixels.resize(width * height * bytesPerPixel);
    gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                     postPixels.data());

    EXPECT_TRUE(ImageMatches(width, height, bytesPerPixel, width,
                             prePixels.data(), postPixels.data()));
}

}  // namespace emugl
