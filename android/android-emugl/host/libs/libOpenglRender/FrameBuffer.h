/*
* Copyright (C) 2011-2015 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef _LIBRENDER_FRAMEBUFFER_H
#define _LIBRENDER_FRAMEBUFFER_H

#include "android/base/files/Stream.h"
#include "android/base/threads/Thread.h"
#include "android/base/threads/WorkerThread.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/snapshot/common.h"
#include "android/base/EventNotificationSupport.h"
#include "android/opengl/virtio_gpu_ops.h"

#include "ColorBuffer.h"
#include "emugl/common/mutex.h"
#include "FbConfig.h"
#include "GLESVersionDetector.h"
#include "Hwc2.h"
#include "PostWorker.h"
#include "ReadbackWorker.h"
#include "RenderContext.h"
#include "TextureDraw.h"
#include "VsyncThread.h"
#include "WindowSurface.h"

#include "OpenglRender/render_api.h"
#include "OpenglRender/Renderer.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <stdint.h>

struct ColorBufferRef {
    ColorBufferPtr cb;
    uint32_t refcount;  // number of client-side references

    // Tracks whether opened at least once. In O+,
    // color buffers can be created/closed immediately,
    // but then registered (opened) afterwards.
    bool opened;

    // Tracks the time when this buffer got a close request while not being
    // opened yet.
    android::base::System::Duration closedTs;
};

struct BufferRef {
    BufferPtr buffer;
};



typedef std::unordered_map<HandleType, std::pair<WindowSurfacePtr, HandleType> > WindowSurfaceMap;
typedef std::unordered_set<HandleType> WindowSurfaceSet;
typedef std::unordered_map<uint64_t, WindowSurfaceSet> ProcOwnedWindowSurfaces;

typedef std::unordered_map<HandleType, RenderContextPtr> RenderContextMap;
typedef std::unordered_set<HandleType> RenderContextSet;
typedef std::unordered_map<uint64_t, RenderContextSet> ProcOwnedRenderContexts;

typedef std::unordered_map<HandleType, ColorBufferRef> ColorBufferMap;
typedef std::unordered_multiset<HandleType> ColorBufferSet;
typedef std::unordered_map<uint64_t, ColorBufferSet> ProcOwnedColorBuffers;

typedef std::unordered_map<HandleType, BufferRef> BufferMap;
typedef std::unordered_multiset<HandleType> BufferSet;
typedef std::unordered_map<uint64_t, BufferSet> ProcOwnedBuffers;

typedef std::unordered_set<HandleType> EGLImageSet;
typedef std::unordered_map<uint64_t, EGLImageSet> ProcOwnedEGLImages;

typedef std::unordered_map<void*, std::function<void()>> CallbackMap;
typedef std::unordered_map<uint64_t, CallbackMap> ProcOwnedCleanupCallbacks;

typedef std::unordered_map<uint64_t, uint32_t*> ProcOwnedSequenceNumbers;

// A structure used to list the capabilities of the underlying EGL
// implementation that the FrameBuffer instance depends on.
// |has_eglimage_texture_2d| is true iff the EGL_KHR_gl_texture_2D_image
// extension is supported.
// |has_eglimage_renderbuffer| is true iff the EGL_KHR_gl_renderbuffer_image
// extension is supported.
// |eglMajor| and |eglMinor| are the major and minor version numbers of
// the underlying EGL implementation.
struct FrameBufferCaps {
    bool has_eglimage_texture_2d;
    bool has_eglimage_renderbuffer;
    EGLint eglMajor;
    EGLint eglMinor;
};

// The FrameBuffer class holds the global state of the emulation library on
// top of the underlying EGL/GLES implementation. It should probably be
// named "Display" instead of "FrameBuffer".
//
// There is only one global instance, that can be retrieved with getFB(),
// and which must be previously setup by calling initialize().
//
// You can register a listener to be informed of FrameBufferChange events. The registered
// callback will be invoked whenever a new frame has been made available. Listeners should
// do as little as possible as the callback is on the render thread itself.
class FrameBuffer :  public android::base::EventNotificationSupport<emugl::FrameBufferChangeEvent> {
public:
    // Initialize the global instance.
    // |width| and |height| are the dimensions of the emulator GPU display
    // in pixels. |useSubWindow| is true to indicate that the caller
    // will use setupSubWindow() to let EmuGL display the GPU content in its
    // own sub-windows. If false, this means the caller will use
    // setPostCallback() instead to retrieve the content.
    // Returns true on success, false otherwise.
    static bool initialize(int width, int height, bool useSubWindow,
            bool egl2egl);

    // Setup a sub-window to display the content of the emulated GPU
    // on-top of an existing UI window. |p_window| is the platform-specific
    // parent window handle. |wx|, |wy|, |ww| and |wh| are the
    // dimensions in pixels of the sub-window, relative to the parent window's
    // coordinate. |fbw| and |fbh| are the dimensions used to initialize
    // the framebuffer, which may be different from the dimensions of the
    // sub-window (in which case scaling will be applied automatically).
    // |dpr| is the device pixel ratio of the monitor, which is needed for
    // proper panning on high-density displays (like retina)
    // |zRot| is a rotation angle in degrees, (clockwise in the Y-upwards GL
    // coordinate space).
    //
    // If a sub-window already exists, this function updates the subwindow
    // and framebuffer properties to match the given values.
    //
    // Return true on success, false otherwise.
    //
    // NOTE: This can return false for software-only EGL engines like OSMesa.
    bool setupSubWindow(FBNativeWindowType p_window,
                        int wx,
                        int wy,
                        int ww,
                        int wh,
                        int fbw,
                        int fbh,
                        float dpr,
                        float zRot,
                        bool deleteExisting,
                        bool hideWindow);

    // Remove the sub-window created by setupSubWindow(), if any.
    // Return true on success, false otherwise.
    bool removeSubWindow();

    // Finalize the instance.
    void finalize();

    // Return a pointer to the global instance. initialize() must be called
    // previously, or this will return NULL.
    static FrameBuffer *getFB() { return s_theFrameBuffer; }

    // Wait for a FrameBuffer instance to be initialized and ready to use.
    // This function blocks the caller until there is a valid initialized
    // object in getFB() and
    static void waitUntilInitialized();

    // Return the capabilities of the underlying display.
    const FrameBufferCaps &getCaps() const { return m_caps; }

    // Return the emulated GPU display width in pixels.
    int getWidth() const { return m_framebufferWidth; }

    // Return the emulated GPU display height in pixels.
    int getHeight() const { return m_framebufferHeight; }

    // Return the list of configs available from this display.
    const FbConfigList* getConfigs() const { return m_configs; }

    // Set a callback that will be called each time the emulated GPU content
    // is updated. This can be relatively slow with host-based GPU emulation,
    // so only do this when you need to.
    void setPostCallback(emugl::Renderer::OnPostCallback onPost,
                         void* onPostContext,
                         uint32_t displayId,
                         bool useBgraReadback = false);

    // Retrieve the GL strings of the underlying EGL/GLES implementation.
    // On return, |*vendor|, |*renderer| and |*version| will point to strings
    // that are owned by the instance (and must not be freed by the caller).
    void getGLStrings(const char** vendor,
                      const char** renderer,
                      const char** version) const {
        *vendor = m_glVendor.c_str();
        *renderer = m_glRenderer.c_str();
        *version = m_glVersion.c_str();
    }

    // Create a new RenderContext instance for this display instance.
    // |p_config| is the index of one of the configs returned by getConfigs().
    // |p_share| is either EGL_NO_CONTEXT or the handle of a shared context.
    // |version| specifies the GLES version as a GLESApi enum.
    // Return a new handle value, which will be 0 in case of error.
    HandleType createRenderContext(int p_config, HandleType p_share,
        GLESApi version = GLESApi_CM);

    // Create a new WindowSurface instance from this display instance.
    // |p_config| is the index of one of the configs returned by getConfigs().
    // |p_width| and |p_height| are the window dimensions in pixels.
    // Return a new handle value, or 0 in case of error.
    HandleType createWindowSurface(int p_config, int p_width, int p_height);

    // Create a new ColorBuffer instance from this display instance.
    // |p_width| and |p_height| are its dimensions in pixels.
    // |p_internalFormat| is the OpenGL format of this color buffer.
    // |p_frameworkFormat| describes the Android frameework format of this
    // color buffer, if differing from |p_internalFormat|.
    // See ColorBuffer::create() for
    // list of valid values. Note that ColorBuffer instances are reference-
    // counted. Use openColorBuffer / closeColorBuffer to operate on the
    // internal count.
    HandleType createColorBuffer(
        int p_width, int p_height, GLenum p_internalFormat,
        FrameworkFormat p_frameworkFormat);
    // Variant of createColorBuffer except with a particular
    // handle already assigned. This is for use with
    // virtio-gpu's RESOURCE_CREATE ioctl.
    void createColorBufferWithHandle(
        int p_width, int p_height, GLenum p_internalFormat,
        FrameworkFormat p_frameworkFormat,
        HandleType handle);

    // Create a new data Buffer instance from this display instance.
    // The buffer will be backed by a VkBuffer and VkDeviceMemory (if Vulkan
    // is available).
    // |size| is the requested size of Buffer in bytes.
    // |memoryProperty| is the requested memory property bits of the device
    // memory.
    HandleType createBuffer(uint64_t size, uint32_t memoryProperty);

    // Call this function when a render thread terminates to destroy all
    // the remaining contexts it created. Necessary to avoid leaking host
    // contexts when a guest application crashes, for example.
    void drainRenderContext();

    // Call this function when a render thread terminates to destroy all
    // remaining window surfqce it created. Necessary to avoid leaking
    // host buffers when a guest application crashes, for example.
    void drainWindowSurface();

    // Destroy a given RenderContext instance. |p_context| is its handle
    // value as returned by createRenderContext().
    void DestroyRenderContext(HandleType p_context);

    // Destroy a given WindowSurface instance. |p_surcace| is its handle
    // value as returned by createWindowSurface().
    void DestroyWindowSurface(HandleType p_surface);
    // Returns the set of ColorBuffers destroyed (for further cleanup)
    std::vector<HandleType> DestroyWindowSurfaceLocked(HandleType p_surface);

    // Increment the reference count associated with a given ColorBuffer
    // instance. |p_colorbuffer| is its handle value as returned by
    // createColorBuffer().
    int openColorBuffer(HandleType p_colorbuffer);

    // Decrement the reference count associated with a given ColorBuffer
    // instance. |p_colorbuffer| is its handle value as returned by
    // createColorBuffer(). Note that if the reference count reaches 0,
    // the instance is destroyed automatically.
    void closeColorBuffer(HandleType p_colorbuffer);

    // Destroy a Buffer created previously. |p_buffer| is its handle value as
    // returned by createBuffer().
    void closeBuffer(HandleType p_colorbuffer);

    void cleanupProcGLObjects(uint64_t puid);

    // Equivalent for eglMakeCurrent() for the current display.
    // |p_context|, |p_drawSurface| and |p_readSurface| are the handle values
    // of the context, the draw surface and the read surface, respectively.
    // Returns true on success, false on failure.
    // Note: if all handle values are 0, this is an unbind operation.
    bool  bindContext(HandleType p_context,
                      HandleType p_drawSurface,
                      HandleType p_readSurface);

    // Return a render context pointer from its handle
    RenderContextPtr getContext_locked(HandleType p_context);

    // Return a color buffer pointer from its handle
    ColorBufferPtr getColorBuffer_locked(HandleType p_colorBuffer);

    // Return a color buffer pointer from its handle
    WindowSurfacePtr getWindowSurface_locked(HandleType p_windowsurface);

    // Attach a ColorBuffer to a WindowSurface instance.
    // See the documentation for WindowSurface::setColorBuffer().
    // |p_surface| is the target WindowSurface's handle value.
    // |p_colorbuffer| is the ColorBuffer handle value.
    // Returns true on success, false otherwise.
    bool  setWindowSurfaceColorBuffer(
            HandleType p_surface, HandleType p_colorbuffer);

    // Copy the content of a WindowSurface's Pbuffer to its attached
    // ColorBuffer. See the documentation for WindowSurface::flushColorBuffer()
    // |p_surface| is the target WindowSurface's handle value.
    // Returns true on success, false on failure.
    bool  flushWindowSurfaceColorBuffer(HandleType p_surface);

    // Retrieves the color buffer handle associated with |p_surface|.
    // Returns 0 if there is no such handle.
    HandleType getWindowSurfaceColorBufferHandle(HandleType p_surface);

    // Bind the current context's EGL_TEXTURE_2D texture to a ColorBuffer
    // instance's EGLImage. This is intended to implement
    // glEGLImageTargetTexture2DOES() for all GLES versions.
    // |p_colorbuffer| is the ColorBuffer's handle value.
    // Returns true on success, false on failure.
    bool  bindColorBufferToTexture(HandleType p_colorbuffer);
    bool  bindColorBufferToTexture2(HandleType p_colorbuffer);

    // Bind the current context's EGL_RENDERBUFFER_OES render buffer to this
    // ColorBuffer's EGLImage. This is intended to implement
    // glEGLImageTargetRenderbufferStorageOES() for all GLES versions.
    // |p_colorbuffer| is the ColorBuffer's handle value.
    // Returns true on success, false on failure.
    bool  bindColorBufferToRenderbuffer(HandleType p_colorbuffer);

    // Read the content of a given ColorBuffer into client memory.
    // |p_colorbuffer| is the ColorBuffer's handle value. Similar
    // to glReadPixels(), this can be a slow operation.
    // |x|, |y|, |width| and |height| are the position and dimensions of
    // a rectangle whose pixel values will be transfered to the host.
    // |format| indicates the format of the pixel data, e.g. GL_RGB or GL_RGBA.
    // |type| is the type of pixel data, e.g. GL_UNSIGNED_BYTE.
    // |pixels| is the address of a caller-provided buffer that will be filled
    // with the pixel data.
    void  readColorBuffer(HandleType p_colorbuffer,
                           int x, int y, int width, int height,
                           GLenum format, GLenum type, void *pixels);

    // Read the content of a given YUV420_888 ColorBuffer into client memory.
    // |p_colorbuffer| is the ColorBuffer's handle value. Similar
    // to glReadPixels(), this can be a slow operation.
    // |x|, |y|, |width| and |height| are the position and dimensions of
    // a rectangle whose pixel values will be transfered to the host.
    // |pixels| is the address of a caller-provided buffer that will be filled
    // with the pixel data.
    // |pixles_size| is the size of buffer
    void  readColorBufferYUV(HandleType p_colorbuffer,
                             int x, int y, int width, int height,
                             void *pixels, uint32_t pixels_size);

    // create a Y texture and a UV texture with width and height, the created
    // texture ids are stored in textures respectively
    void createYUVTextures(uint32_t type,
                           uint32_t count,
                           int width,
                           int height,
                           uint32_t* output);
    void destroyYUVTextures(uint32_t type, uint32_t count, uint32_t* textures);
    void updateYUVTextures(uint32_t type,
                           uint32_t* textures,
                           void* privData,
                           void* func);
    void swapTexturesAndUpdateColorBuffer(uint32_t colorbufferhandle,
                                          int x,
                                          int y,
                                          int width,
                                          int height,
                                          uint32_t format,
                                          uint32_t type,
                                          uint32_t texture_type,
                                          uint32_t* textures);

    // Update the content of a given ColorBuffer from client data.
    // |p_colorbuffer| is the ColorBuffer's handle value. Similar
    // to glReadPixels(), this can be a slow operation.
    // |x|, |y|, |width| and |height| are the position and dimensions of
    // a rectangle whose pixel values will be transfered to the GPU
    // |format| indicates the format of the OpenGL buffer, e.g. GL_RGB or GL_RGBA.
    // |frameworkFormat| indicates the format of the pixel data; if
    // FRAMEWORK_FORMAT_GL_COMPATIBLE, |format| (OpenGL format) is used.
    // Otherwise, explicit conversion to |format| is needed.
    // |type| is the type of pixel data, e.g. GL_UNSIGNED_BYTE.
    // |pixels| is the address of a buffer containing the new pixel data.
    // Returns true on success, false otherwise.
    bool updateColorBuffer(HandleType p_colorbuffer,
                           int x, int y, int width, int height,
                           GLenum format, GLenum type, void *pixels);
    bool updateColorBufferFromFrameworkFormat(HandleType p_colorbuffer,
                           int x, int y, int width, int height,
                           FrameworkFormat fwkFormat, GLenum format, GLenum type, void *pixels);
    // Replaces contents completely using the color buffer's current format,
    // with row length equal to width of a row in bytes.
    // The number of bytes is passed as a check.
    bool replaceColorBufferContents(
        HandleType p_colorbuffer,
        const void *pixels,
        size_t numBytes);
    // Reads back the raw color buffer to |pixels|
    // if |pixels| is not null.
    // Always returns in |numBytes| how many bytes were
    // planned to be transmitted.
    // |numBytes| is not an input parameter;
    // fewer or more bytes cannot be specified.
    // If the framework format is YUV, it will read
    // back as raw YUV data.
    bool readColorBufferContents(
        HandleType p_colorbuffer,
        size_t* numBytes,
        void *pixels);

    bool getColorBufferInfo(HandleType p_colorbuffer,
                            int* width,
                            int* height,
                            GLint* internalformat,
                            FrameworkFormat* frameworkFormat = nullptr);
    bool getBufferInfo(HandleType p_buffer, int* size);

    // Display the content of a given ColorBuffer into the framebuffer's
    // sub-window. |p_colorbuffer| is a handle value.
    // |needLockAndBind| is used to indicate whether the operation requires
    // acquiring/releasing the FrameBuffer instance's lock and binding the
    // contexts. It should be |false| only when called internally.
    bool post(HandleType p_colorbuffer, bool needLockAndBind = true);
    bool hasGuestPostedAFrame() { return m_guestPostedAFrame; }
    void resetGuestPostedAFrame() { m_guestPostedAFrame = false; }

    // Runs the post callback with |pixels| (good for when the readback
    // happens in a separate place)
    void doPostCallback(void* pixels, uint32_t displayId);

    void getPixels(void* pixels, uint32_t bytes, uint32_t displayId);
    void flushReadPipeline(int displayId);
    void ensureReadbackWorker();

    bool asyncReadbackSupported();
    emugl::Renderer::ReadPixelsCallback getReadPixelsCallback();
    emugl::Renderer::FlushReadPixelPipeline getFlushReadPixelPipeline();

    // Re-post the last ColorBuffer that was displayed through post().
    // This is useful if you detect that the sub-window content needs to
    // be re-displayed for any reason.
    bool repost(bool needLockAndBind = true);

    // Return the host EGLDisplay used by this instance.
    EGLDisplay getDisplay() const { return m_eglDisplay; }
    EGLSurface getWindowSurface() const { return m_eglSurface; }
    EGLContext getContext() const { return m_eglContext; }
    EGLConfig getConfig() const { return m_eglConfig; }

    // Change the rotation of the displayed GPU sub-window.
    void setDisplayRotation(float zRot) {
        if (zRot != m_zRot) {
            m_zRot = zRot;
            repost();
        }
    }

    // Changes what coordinate of this framebuffer will be displayed at the
    // corner of the GPU sub-window. Specifically, |px| and |py| = 0 means
    // align the bottom-left of the framebuffer with the bottom-left of the
    // sub-window, and |px| and |py| = 1 means align the top right of the
    // framebuffer with the top right of the sub-window. Intermediate values
    // interpolate between these states.
    void setDisplayTranslation(float px, float py) {
        // Sanity check the values to ensure they are between 0 and 1
        const float x = px > 1.f ? 1.f : (px < 0.f ? 0.f : px);
        const float y = py > 1.f ? 1.f : (py < 0.f ? 0.f : py);
        if (x != m_px || y != m_py) {
            m_px = x;
            m_py = y;
            repost();
        }
    }

    // Return a TextureDraw instance that can be used with this surfaces
    // and windows created by this instance.
    TextureDraw* getTextureDraw() const { return m_textureDraw; }

    // Create an eglImage and return its handle.  Reference:
    // https://www.khronos.org/registry/egl/extensions/KHR/EGL_KHR_image_base.txt
    HandleType createClientImage(HandleType context, EGLenum target, GLuint buffer);
    // Call the implementation of eglDestroyImageKHR, return if succeeds or
    // not. Reference:
    // https://www.khronos.org/registry/egl/extensions/KHR/EGL_KHR_image_base.txt
    EGLBoolean destroyClientImage(HandleType image);

    // Used internally.
    bool bind_locked();
    bool unbind_locked();

    void lockContextStructureRead() { m_contextStructureLock.lockRead(); }
    void unlockContextStructureRead() { m_contextStructureLock.unlockRead(); }

    // For use with sync threads and otherwise, any time we need a GL context
    // not specifically for drawing, but to obtain certain things about
    // GL state.
    // It can be unsafe / leaky to change the structure of contexts
    // outside the facilities the FrameBuffer class provides.
    void getTrivialContextForCurrentRenderThread(HandleType shared,
                                                 HandleType* contextOut,
                                                 HandleType* surfOut);
    // createTrivialContext(), but with a m_pbufContext
    // as shared, and not adding itself to the context map at all.
    void createSharedTrivialContext(EGLContext* contextOut, EGLSurface* surfOut);
    void destroySharedTrivialContext(EGLContext context, EGLSurface surf);

    void setShuttingDown() { m_shuttingDown = true; }
    bool isShuttingDown() const { return m_shuttingDown; }
    bool compose(uint32_t bufferSize, void* buffer, bool post = true);

    ~FrameBuffer();

    void onSave(android::base::Stream* stream,
                const android::snapshot::ITextureSaverPtr& textureSaver);
    bool onLoad(android::base::Stream* stream,
                const android::snapshot::ITextureLoaderPtr& textureLoader);

    // lock and unlock handles (RenderContext, ColorBuffer, WindowSurface)
    void lock();
    void unlock();

    static void setMaxGLESVersion(GLESDispatchMaxVersion version);
    static GLESDispatchMaxVersion getMaxGLESVersion();

    float getDpr() const { return m_dpr; }
    int windowWidth() const { return m_windowWidth; }
    int windowHeight() const { return m_windowHeight; }
    float getPx() const { return m_px; }
    float getPy() const { return m_py; }
    int getZrot() const { return m_zRot; }

    bool isFastBlitSupported() const { return m_fastBlitSupported; }
    bool isVulkanInteropSupported() const { return m_vulkanInteropSupported; }
    bool importMemoryToColorBuffer(
#ifdef _WIN32
        void* handle,
#else
        int handle,
#endif
        uint64_t size,
        bool dedicated,
        bool linearTiling,
        bool vulkanOnly,
        uint32_t colorBufferHandle);
    void setColorBufferInUse(uint32_t colorBufferHandle, bool inUse);

    // Used during tests to disable fast blit.
    void disableFastBlit();

    // Fill GLES usage protobuf
    void fillGLESUsages(android_studio::EmulatorGLESUsages*);

    // Saves a screenshot of the previous frame.
    // nChannels should be 3 (RGB) or 4 (RGBA).
    // You must provide a pre-allocated buffer of sufficient
    // size. Returns 0 on success. In the case of failure and if *cPixels != 0
    // you can call this function again with a buffer of size *cPixels. cPixels
    // should usually be at at least desiredWidth * desiredHeight * nChannels.
    //
    // In practice the buffer should be > desiredWidth *
    // desiredHeight * nChannels.
    //
    // Note: Do not call this function again if it fails and *cPixels == 0
    //  swiftshader_indirect does not work with 3 channels
    //
    // This function supports rectangle snipping by
    // providing an |rect| parameter. The default value of {{0,0}, {0,0}}
    // indicates the users wants to snip the entire screen instead of a
    // partial screen.
    // - |rect|  represents a rectangle within the screen defined by
    // desiredWidth and desiredHeight.
    int getScreenshot(unsigned int nChannels,
                      unsigned int* width,
                      unsigned int* height,
                      uint8_t* pixels,
                      size_t* cPixels,
                      int displayId,
                      int desiredWidth,
                      int desiredHeight,
                      SkinRotation desiredRotation,
                      SkinRect rect = {{0, 0}, {0, 0}});

    void onLastColorBufferRef(uint32_t handle);
    ColorBuffer::Helper* getColorBufferHelper() { return m_colorBufferHelper; }
    ColorBufferPtr findColorBuffer(HandleType p_colorbuffer);

    void registerProcessCleanupCallback(void* key, std::function<void()> callback);
    void unregisterProcessCleanupCallback(void* key);

    void registerProcessSequenceNumberForPuid(uint64_t puid);
    uint32_t* getProcessSequenceNumberPtr(uint64_t puid);

    int createDisplay(uint32_t *displayId);
    int createDisplay(uint32_t displayId);
    int destroyDisplay(uint32_t displayId);
    int setDisplayColorBuffer(uint32_t displayId, uint32_t colorBuffer);
    int getDisplayColorBuffer(uint32_t displayId, uint32_t* colorBuffer);
    int getColorBufferDisplay(uint32_t colorBuffer, uint32_t* displayId);
    int getDisplayPose(uint32_t displayId,
                       int32_t* x,
                       int32_t* y,
                       uint32_t* w,
                       uint32_t* h);
    int setDisplayPose(uint32_t displayId,
                       int32_t x,
                       int32_t y,
                       uint32_t w,
                       uint32_t h,
                       uint32_t dpi = 0);
    void getCombinedDisplaySize(int* w, int* h);
    struct DisplayInfo {
        uint32_t cb;
        int32_t pos_x;
        int32_t pos_y;
        uint32_t width;
        uint32_t height;
        uint32_t dpi;
        DisplayInfo() : cb(0), pos_x(0), pos_y(0), width(0), height(0), dpi(0) {};
        DisplayInfo(uint32_t cb, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t d)
            : cb(cb), pos_x(x), pos_y(y), width(w), height(h), dpi(d) {}
    };
    // Inline with MultiDisplay::s_invalidIdMultiDisplay
    static const uint32_t s_invalidIdMultiDisplay = 0xFFFFFFAB;
    static const uint32_t s_maxNumMultiDisplay = 11;

    EGLContext getGlobalEGLContext() { return m_pbufContext; }
    HandleType getLastPostedColorBuffer() { return m_lastPostedColorBuffer; }
    void waitForGpu(uint64_t eglsync);
    void waitForGpuVulkan(uint64_t deviceHandle, uint64_t fenceHandle);
    void asyncWaitForGpuWithCb(uint64_t eglsync, FenceCompletionCallback cb);
    void asyncWaitForGpuVulkanWithCb(uint64_t deviceHandle, uint64_t fenceHandle, FenceCompletionCallback cb);
    void asyncWaitForGpuVulkanQsriWithCb(uint64_t image, FenceCompletionCallback cb);
    void waitForGpuVulkanQsri(uint64_t image);

    bool platformImportResource(uint32_t handle, uint32_t type, void* resource);
    void* platformCreateSharedEglContext(void);
    bool platformDestroySharedEglContext(void* context);

    void setGuestManagedColorBufferLifetime(bool guestManaged);

    void setVsyncHz(int vsyncHz);
    void scheduleVsyncTask(VsyncThread::VsyncTask task);
    void setDisplayConfigs(int configId, int w, int h, int dpiX, int dpiY);
    void setDisplayActiveConfig(int configId);
    const int getDisplayConfigsCount();
    const int getDisplayConfigsParam(int configId, EGLint param);
    const int getDisplayActiveConfig();

private:
    FrameBuffer(int p_width, int p_height, bool useSubWindow);
    HandleType genHandle_locked();

    bool bindSubwin_locked();
    bool bindFakeWindow_locked();
    bool removeSubWindow_locked();
    // Returns the set of ColorBuffers destroyed (for further cleanup)
    std::vector<HandleType> cleanupProcGLObjects_locked(uint64_t puid, bool forced = false);

    void markOpened(ColorBufferRef* cbRef);
    // Returns true if the color buffer was erased.
    bool closeColorBufferLocked(HandleType p_colorbuffer, bool forced = false);
    // Returns true if this was the last ref and we need to destroy stuff.
    bool decColorBufferRefCountLocked(HandleType p_colorbuffer);
    // Close all expired color buffers for real.
    // Treat all delayed color buffers as expired if forced=true
    void performDelayedColorBufferCloseLocked(bool forced = false);
    void eraseDelayedCloseColorBufferLocked(
            HandleType cb, android::base::System::Duration ts);

    bool postImpl(HandleType p_colorbuffer, bool needLockAndBind = true, bool repaint = false);
    void setGuestPostedAFrame() {
        m_guestPostedAFrame = true;
        fireEvent({ emugl::FrameBufferChange::FrameReady,  mFrameNumber++ });
    }
    HandleType createColorBufferLocked(int p_width,
                                       int p_height,
                                       GLenum p_internalFormat,
                                       FrameworkFormat p_frameworkFormat);
    HandleType createColorBufferWithHandleLocked(
        int p_width,
        int p_height,
        GLenum p_internalFormat,
        FrameworkFormat p_frameworkFormat,
        HandleType handle);
    HandleType createBufferLocked(int p_size);
    HandleType createBufferWithHandleLocked(int p_size, HandleType handle);

    void recomputeLayout();
    void setDisplayPoseInSkinUI(int totalHeight);
    void sweepColorBuffersLocked();

private:
    static FrameBuffer *s_theFrameBuffer;
    static HandleType s_nextHandle;
    int m_x = 0;
    int m_y = 0;
    int m_framebufferWidth = 0;
    int m_framebufferHeight = 0;
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    float m_dpr = 0;

    bool m_useSubWindow = false;
    bool m_eglContextInitialized = false;

    bool m_fpsStats = false;
    bool m_perfStats = false;
    int m_statsNumFrames = 0;
    long long m_statsStartTime = 0;

    android::base::Thread* m_perfThread;
    uint64_t mFrameNumber;
    emugl::Mutex m_lock;
    emugl::ReadWriteMutex m_contextStructureLock;
    FbConfigList* m_configs = nullptr;
    FBNativeWindowType m_nativeWindow = 0;
    FrameBufferCaps m_caps = {};
    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    RenderContextMap m_contexts;
    WindowSurfaceMap m_windows;
    ColorBufferMap m_colorbuffers;
    BufferMap m_buffers;
    std::unordered_map<HandleType, HandleType> m_windowSurfaceToColorBuffer;

    // A collection of color buffers that were closed without any usages
    // (|opened| == false).
    //
    // If a buffer reached |refcount| == 0 while not being |opened|, instead of
    // deleting it we remember the timestamp when this happened. Later, we
    // check if the buffer stayed unopened long enough and if it did, we delete
    // it permanently. On the other hand, if the color buffer was used then
    // we don't care about timestamps anymore.
    //
    // Note: this collection is ordered by |ts| field.
    struct ColorBufferCloseInfo {
        android::base::System::Duration ts; // when we got the close request.
        HandleType cbHandle;    // 0 == already closed, do nothing
    };
    using ColorBufferDelayedClose = std::vector<ColorBufferCloseInfo>;
    ColorBufferDelayedClose m_colorBufferDelayedCloseList;

    ColorBuffer::Helper* m_colorBufferHelper = nullptr;

    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLContext m_eglContext = EGL_NO_CONTEXT;
    EGLSurface m_pbufSurface = EGL_NO_SURFACE;
    EGLContext m_pbufContext = EGL_NO_CONTEXT;

    EGLSurface m_eglFakeWindowSurface = EGL_NO_SURFACE;
    EGLContext m_eglFakeWindowContext = EGL_NO_CONTEXT;

    EGLContext m_prevContext = EGL_NO_CONTEXT;
    EGLSurface m_prevReadSurf = EGL_NO_SURFACE;
    EGLSurface m_prevDrawSurf = EGL_NO_SURFACE;
    EGLNativeWindowType m_subWin = {};
    TextureDraw* m_textureDraw = nullptr;
    EGLConfig  m_eglConfig = nullptr;
    HandleType m_lastPostedColorBuffer = 0;
    float      m_zRot = 0;
    float      m_px = 0;
    float      m_py = 0;

    // Async readback
    enum class ReadbackCmd {
        Init = 0,
        GetPixels = 1,
        AddRecordDisplay = 2,
        DelRecordDisplay = 3,
        Exit = 4,
    };
    struct Readback {
        ReadbackCmd cmd;
        uint32_t displayId;
        GLuint bufferId;
        void* pixelsOut;
        uint32_t bytes;
        uint32_t width;
        uint32_t height;
    };
    android::base::WorkerProcessingResult sendReadbackWorkerCmd(const Readback& readback);
    bool m_asyncReadbackSupported = true;
    bool m_guestPostedAFrame = false;

    struct onPost {
        emugl::Renderer::OnPostCallback cb;
        void* context;
        uint32_t displayId;
        uint32_t width;
        uint32_t height;
        unsigned char* img = nullptr;
        bool readBgra;
        ~onPost() {
            if (img) {
                delete[] img;
                img = nullptr;
            }
        }
    };
    std::map<uint32_t, onPost> m_onPost;
    std::unique_ptr<ReadbackWorker> m_readbackWorker;
    android::base::WorkerThread<Readback> m_readbackThread;

    std::string m_glVendor;
    std::string m_glRenderer;
    std::string m_glVersion;

    // The host associates color buffers with guest processes for memory
    // cleanup. Guest processes are identified with a host generated unique ID.
    ProcOwnedWindowSurfaces m_procOwnedWindowSurfaces;
    ProcOwnedColorBuffers m_procOwnedColorBuffers;
    ProcOwnedEGLImages m_procOwnedEGLImages;
    ProcOwnedRenderContexts m_procOwnedRenderContext;
    ProcOwnedCleanupCallbacks m_procOwnedCleanupCallbacks;
    ProcOwnedSequenceNumbers m_procOwnedSequenceNumbers;

    // Flag set when emulator is shutting down.
    bool m_shuttingDown = false;

    // When this feature is enabled, open/close operations from gralloc in guest
    // will no longer control the reference counting of color buffers on host.
    // Instead, it will be managed by a file descriptor in the guest kernel. In
    // case all the native handles in guest are destroyed, the pipe will be
    // automatically closed by the kernel. We only need to do reference counting
    // for color buffers attached in window surface.
    bool m_refCountPipeEnabled = false;

    // When this feature is enabled, and m_refCountPipeEnabled == false, color
    // buffer close operations will immediately close the color buffer if host
    // refcount hits 0. This is for use with guest kernels where the color
    // buffer is already tied to a file descriptor in the guest kernel.
    bool m_noDelayCloseColorBufferEnabled = false;

    // Posting
    enum class PostCmd {
        Post = 0,
        Viewport = 1,
        Compose = 2,
        Clear = 3,
        Screenshot = 4,
        Exit = 5,
    };

    struct Post {
        PostCmd cmd;
        int composeVersion;
        std::vector<char> composeBuffer;
        union {
            HandleType cb;
            struct {
                int width;
                int height;
            } viewport;
            struct {
                ColorBuffer* cb;
                int screenwidth;
                int screenheight;
                GLenum format;
                GLenum type;
                SkinRotation rotation;
                void* pixels;
                SkinRect rect;
            } screenshot;
        };
    };

    std::unique_ptr<PostWorker> m_postWorker = {};
    android::base::WorkerThread<Post> m_postThread;
    android::base::WorkerProcessingResult postWorkerFunc(const Post& post);
    void sendPostWorkerCmd(Post post);

    bool m_fastBlitSupported = false;
    bool m_vulkanInteropSupported = false;
    bool m_guestUsesAngle = false;
    // Whether the guest manages ColorBuffer lifetime
    // so we don't need refcounting on the host side.
    bool m_guestManagedColorBufferLifetime = false;

    android::base::MessageChannel<HandleType, 1024>
        mOutstandingColorBufferDestroys;

    // UUIDs of physical devices for Vulkan and GLES, respectively.  In most
    // cases, this determines whether we can support zero-copy interop.
    uint8_t m_vulkanUUID[VK_UUID_SIZE];
    uint8_t m_glesUUID[GL_UUID_SIZE_EXT];
    static_assert(VK_UUID_SIZE == GL_UUID_SIZE_EXT);

    int m_vsyncHz = 60;

    // Vsync thread.
    std::unique_ptr<VsyncThread> m_vsyncThread = {};

    struct DisplayConfig{
        int w;
        int h;
        int dpiX;
        int dpiY;
        DisplayConfig() {}
        DisplayConfig(int w, int h, int x, int y)
        : w(w), h(h), dpiX(x), dpiY(y) {}
    };
    std::map<int, DisplayConfig> mDisplayConfigs;
    int mDisplayActiveConfigId = -1;

    // Tracks platform EGL contexts that have been handed out to other users,
    // indexed by underlying native EGL context object.
    struct PlatformEglContextInfo {
        EGLContext context;
        EGLSurface surface;
    };
    std::unordered_map<void*, PlatformEglContextInfo> m_platformEglContexts;
};
#endif
