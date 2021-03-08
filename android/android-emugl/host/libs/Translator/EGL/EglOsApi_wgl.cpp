/*
* Copyright (C) 2011 The Android Open Source Project
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
#include "EglOsApi.h"

#include "android/base/synchronization/Lock.h"

#include "CoreProfileConfigs.h"
#include "emugl/common/lazy_instance.h"
#include "emugl/common/logging.h"
#include "emugl/common/shared_library.h"
#include "emugl/common/thread_store.h"
#include "GLcommon/GLLibrary.h"

#include "OpenglCodecCommon/ErrorLog.h"

#include <windows.h>
#include <wingdi.h>

#include <GLES/glplatform.h>
#include <GL/gl.h>
#include <GL/wglext.h>

#include <EGL/eglext.h>

#include <unordered_map>
#include <unordered_set>

#include <stdio.h>
#include <stdlib.h>

#define IS_TRUE(a) \
        do { if (!(a)) return NULL; } while (0)

#define EXIT_IF_FALSE(a) \
        do { if (!(a)) return; } while (0)

#define DEBUG 0
#if DEBUG
#define D(...)  fprintf(stderr, __VA_ARGS__)
#else
#define D(...)  ((void)0)
#endif

#define WGL_ERR(...)  do { \
    fprintf(stderr, __VA_ARGS__); \
    GL_LOG(__VA_ARGS__); \
} while(0) \

// TODO: Replace with latency tracker.
#define PROFILE_SLOW(tag)

namespace {

using emugl::SharedLibrary;
typedef GlLibrary::GlFunctionPointer GlFunctionPointer;

// Returns true if an extension is include in a given extension list.
// |extension| is an GL extension name.
// |extensionList| is a space-separated list of supported extension.
// Returns true if the extension is supported, false otherwise.
bool supportsExtension(const char* extension, const char* extensionList) {
    size_t extensionLen = ::strlen(extension);
    const char* list = extensionList;
    for (;;) {
        const char* p = const_cast<const char*>(::strstr(list, extension));
        if (!p) {
            return false;
        }
        // Check that the extension appears as a single word in the list
        // i.e. that it is wrapped by either spaces or the start/end of
        // the list.
        if ((p == extensionList || p[-1] == ' ') &&
            (p[extensionLen] == '\0' || p[extensionLen] == ' ')) {
            return true;
        }
        // otherwise, skip over the current position to find something else.
        p += extensionLen;
    }
}

/////
/////  W G L   D I S P A T C H   T A B L E S
/////

// A technical note to explain what is going here, trust me, it's important.
//
// I. Library-dependent symbol resolution:
// ---------------------------------------
//
// The code here can deal with two kinds of OpenGL Windows libraries: the
// system-provided opengl32.dll, or alternate software renderers like Mesa
// (e.g. mesa_opengl32.dll).
//
// When using the system library, pixel-format related functions, like
// SetPixelFormat(), are provided by gdi32.dll, and _not_ opengl32.dll
// (even though they are documented as part of the Windows GL API).
//
// These functions must _not_ be used when using alternative renderers.
// Instead, these provide _undocumented_ alternatives with the 'wgl' prefix,
// as in wglSetPixelFormat(), wglDescribePixelFormat(), etc... which
// implement the same calling conventions.
//
// For more details about this, see 5.190 of the Windows OpenGL FAQ at:
// https://www.opengl.org/archives/resources/faq/technical/mswindows.htm
//
// Another good source of information on this topic:
// http://stackoverflow.com/questions/20645706/why-are-functions-duplicated-between-opengl32-dll-and-gdi32-dll
//
// In practice, it means that the code here should resolve 'naked' symbols
// (e.g. 'GetPixelFormat()) when using the system library, and 'prefixed' ones
// (e.g. 'wglGetPixelFormat()) otherwise.
//

// List of WGL functions of interest to probe with GetProcAddress()
#define LIST_WGL_FUNCTIONS(X) \
    X(HGLRC, wglCreateContext, (HDC hdc)) \
    X(BOOL, wglDeleteContext, (HGLRC hglrc)) \
    X(BOOL, wglMakeCurrent, (HDC hdc, HGLRC hglrc)) \
    X(BOOL, wglShareLists, (HGLRC hglrc1, HGLRC hglrc2)) \
    X(HGLRC, wglGetCurrentContext, (void)) \
    X(HDC, wglGetCurrentDC, (void)) \
    X(GlFunctionPointer, wglGetProcAddress, (const char* functionName)) \

// List of WGL functions exported by GDI32 that must be used when using
// the system's opengl32.dll only, i.e. not with a software renderer like
// Mesa. For more information, see 5.190 at:
// And also:
#define LIST_GDI32_FUNCTIONS(X) \
    X(int, ChoosePixelFormat, (HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd)) \
    X(BOOL, SetPixelFormat, (HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR* pfd)) \
    X(int, GetPixelFormat, (HDC hdc)) \
    X(int, DescribePixelFormat, (HDC hdc, int iPixelFormat, UINT nbytes, LPPIXELFORMATDESCRIPTOR ppfd)) \
    X(BOOL, SwapBuffers, (HDC hdc)) \

// Declare a structure containing pointers to all functions listed above,
// and a way to initialize them.
struct WglBaseDispatch {
    // declare all function pointers, followed by a dummy member used
    // to terminate the constructor's initialization list properly.
#define DECLARE_WGL_POINTER(return_type, function_name, signature) \
    return_type (GL_APIENTRY* function_name) signature;
    LIST_WGL_FUNCTIONS(DECLARE_WGL_POINTER)
    LIST_GDI32_FUNCTIONS(DECLARE_WGL_POINTER)
    SharedLibrary* mLib = nullptr;
    bool mIsSystemLib = false;

    // Default Constructor
    WglBaseDispatch() :
#define INIT_WGL_POINTER(return_type, function_name, signature) \
    function_name(),
            LIST_WGL_FUNCTIONS(INIT_WGL_POINTER)
            LIST_GDI32_FUNCTIONS(INIT_WGL_POINTER)
            mIsSystemLib(false) {}

    // Copy constructor
    WglBaseDispatch(const WglBaseDispatch& other) :
#define COPY_WGL_POINTER(return_type, function_name, signature) \
    function_name(other.function_name),
            LIST_WGL_FUNCTIONS(COPY_WGL_POINTER)
            LIST_GDI32_FUNCTIONS(COPY_WGL_POINTER)
            mLib(other.mLib),
            mIsSystemLib(other.mIsSystemLib) {}

    // Initialize the dispatch table from shared library |glLib|, which
    // must point to either the system or non-system opengl32.dll
    // implementation. If |systemLib| is true, this considers the library
    // to be the system one, and will try to find the GDI32 functions
    // like GetPixelFormat() directly. If |systemLib| is false, this will
    // try to load the wglXXX variants (e.g. for Mesa). See technical note
    // above for details.
    // Returns true on success, false otherwise (i.e. if one of the
    // required symbols could not be loaded).
    bool init(SharedLibrary* glLib, bool systemLib) {
        bool result = true;

        mLib = glLib;
        mIsSystemLib = systemLib;

#define LOAD_WGL_POINTER(return_type, function_name, signature) \
    this->function_name = reinterpret_cast< \
            return_type (GL_APIENTRY*) signature>( \
                    glLib->findSymbol(#function_name)); \
    if (!this->function_name) { \
        WGL_ERR("%s: Could not find %s in GL library\n", __FUNCTION__, \
                #function_name); \
        result = false; \
    }

#define LOAD_WGL_GDI32_POINTER(return_type, function_name, signature) \
    this->function_name = reinterpret_cast< \
            return_type (GL_APIENTRY*) signature>( \
                    GetProcAddress(gdi32, #function_name)); \
    if (!this->function_name) { \
        WGL_ERR("%s: Could not find %s in GDI32 library\n", __FUNCTION__, \
                #function_name); \
        result = false; \
    }

#define LOAD_WGL_INNER_POINTER(return_type, function_name, signature) \
    this->function_name = reinterpret_cast< \
            return_type (GL_APIENTRY*) signature>( \
                    glLib->findSymbol("wgl" #function_name)); \
    if (!this->function_name) { \
        WGL_ERR("%s: Could not find %s in GL library\n", __FUNCTION__, \
                "wgl" #function_name); \
        result = false; \
    }

        LIST_WGL_FUNCTIONS(LOAD_WGL_POINTER)
        if (systemLib) {
            HMODULE gdi32 = GetModuleHandle("gdi32.dll");
            LIST_GDI32_FUNCTIONS(LOAD_WGL_GDI32_POINTER)
        } else {
            LIST_GDI32_FUNCTIONS(LOAD_WGL_INNER_POINTER)
        }
        return result;
    }

    // Find a function, using wglGetProcAddress() first, and if that doesn't
    // work, SharedLibrary::findSymbol().
    // |functionName| is the function name.
    // |glLib| is the GL library to probe with findSymbol() is needed.
    GlFunctionPointer findFunction(const char* functionName) const {
        GlFunctionPointer result = this->wglGetProcAddress(functionName);
        if (!result && mLib) {
            result = mLib->findSymbol(functionName);
        }
        return result;
    }
};

// Used internally by createDummyWindow().
LRESULT CALLBACK dummyWndProc(HWND hwnd,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Create a new dummy window, and return its handle.
// Note that the window is 1x1 pixels and not visible, but
// it can be used to create a device context and associated
// OpenGL rendering context. Return NULL on failure.
HWND createDummyWindow() {
    WNDCLASSEX wcx;
    wcx.cbSize = sizeof(wcx);                       // size of structure
    wcx.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // redraw if size changes
    wcx.lpfnWndProc = dummyWndProc;                 // points to window procedure
    wcx.cbClsExtra = 0;                             // no extra class memory
    wcx.cbWndExtra = sizeof(void*);                 // save extra window memory, to store VasWindow instance
    wcx.hInstance = NULL;                           // handle to instance
    wcx.hIcon = NULL;                               // predefined app. icon
    wcx.hCursor = NULL;
    wcx.hbrBackground = NULL;                       // no background brush
    wcx.lpszMenuName =  NULL;                       // name of menu resource
    wcx.lpszClassName = "DummyWin";                 // name of window class
    wcx.hIconSm = (HICON) NULL;                     // small class icon

    RegisterClassEx(&wcx);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
                               "DummyWin",
                               "Dummy",
                               WS_POPUP,
                               0,
                               0,
                               1,
                               1,
                               NULL,
                               NULL,
                               0,0);
    return hwnd;
}

// List of functions defined by the WGL_ARB_extensions_string extension.
#define LIST_extensions_string_FUNCTIONS(X) \
    X(const char*, wglGetExtensionsString, (HDC hdc))

// List of functions defined by the WGL_ARB_pixel_format extension.
#define LIST_pixel_format_FUNCTIONS(X) \
    X(BOOL, wglGetPixelFormatAttribiv, (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int* piAttributes, int* piValues)) \
    X(BOOL, wglGetPixelFormatAttribfv, (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int* piAttributes, FLOAT* pfValues)) \
    X(BOOL, wglChoosePixelFormat, (HDC, const int* piAttribList, const FLOAT* pfAttribList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats)) \

// List of functions defined by the WGL_ARB_make_current_read extension.
#define LIST_make_current_read_FUNCTIONS(X) \
    X(BOOL, wglMakeContextCurrent, (HDC hDrawDC, HDC hReadDC, HGLRC hglrc)) \
    X(HDC, wglGetCurrentReadDC, (void)) \

// List of functions defined by the WGL_ARB_pbuffer extension.
#define LIST_pbuffer_FUNCTIONS(X) \
    X(HPBUFFERARB, wglCreatePbuffer, (HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList)) \
    X(HDC, wglGetPbufferDC, (HPBUFFERARB hPbuffer)) \
    X(int, wglReleasePbufferDC, (HPBUFFERARB hPbuffer, HDC hdc)) \
    X(BOOL, wglDestroyPbuffer, (HPBUFFERARB hPbuffer)) \

// List of functions defined by WGL_ARB_create_context
#define LIST_create_context_FUNCTIONS(X) \
    X(HGLRC, wglCreateContextAttribs, (HDC hDC, HGLRC hshareContext, const int *attribList)) \

// List of functions define by WGL_EXT_swap_control
#define LIST_swap_control_FUNCTIONS(X) \
    X(void, wglSwapInterval, (int)) \
    X(int, wglGetSwapInterval, (void)) \

#define LIST_WGL_EXTENSIONS_FUNCTIONS(X) \
    LIST_pixel_format_FUNCTIONS(X) \
    LIST_make_current_read_FUNCTIONS(X) \
    LIST_pbuffer_FUNCTIONS(X) \
    LIST_create_context_FUNCTIONS(X) \
    LIST_swap_control_FUNCTIONS(X) \

// A structure used to hold pointers to WGL extension functions.
struct WglExtensionsDispatch : public WglBaseDispatch {
public:
    LIST_WGL_EXTENSIONS_FUNCTIONS(DECLARE_WGL_POINTER)
    int dummy;

    // Default constructor
    explicit WglExtensionsDispatch(const WglBaseDispatch& baseDispatch) :
            WglBaseDispatch(baseDispatch),
            LIST_WGL_EXTENSIONS_FUNCTIONS(INIT_WGL_POINTER)
            dummy(0) {}

    // Initialization
    bool init(HDC hdc) {
        // Base initialization happens first.
        bool result = WglBaseDispatch::init(mLib, mIsSystemLib);
        if (!result) {
            return false;
        }

        // Find the list of extensions.
        typedef const char* (GL_APIENTRY* GetExtensionsStringFunc)(HDC hdc);

        const char* extensionList = nullptr;
        GetExtensionsStringFunc wglGetExtensionsString =
                reinterpret_cast<GetExtensionsStringFunc>(
                        this->findFunction("wglGetExtensionsStringARB"));

        bool foundArb = wglGetExtensionsString != nullptr;

        if (wglGetExtensionsString) {
            extensionList = wglGetExtensionsString(hdc);
        }

        bool extensionListArbNull = extensionList == nullptr;
        bool extensionListArbEmpty = !extensionListArbNull && !strcmp(extensionList, "");

        bool foundExt = false;
        bool extensionListExtNull = false;
        bool extensionListExtEmpty = false;

        // wglGetExtensionsStringARB failed, try wglGetExtensionsStringEXT.
        if (!extensionList || !strcmp(extensionList, "")) {
            wglGetExtensionsString =
                reinterpret_cast<GetExtensionsStringFunc>(
                        this->findFunction("wglGetExtensionsStringEXT"));

            foundExt = wglGetExtensionsString != nullptr;

            if (wglGetExtensionsString) {
                extensionList = wglGetExtensionsString(hdc);
            }
        }

        extensionListExtNull = extensionList == nullptr;
        extensionListExtEmpty = !extensionListExtNull && !strcmp(extensionList, "");

        // Both failed, suicide.
        if (!extensionList || !strcmp(extensionList, "")) {
            bool isRemoteSession = GetSystemMetrics(SM_REMOTESESSION);

            WGL_ERR(
                "%s: Could not find wglGetExtensionsString! "
                "arbFound %d listarbNull/empty %d %d "
                "extFound %d extNull/empty %d %d remote %d\n",
                __FUNCTION__,
                foundArb,
                extensionListArbNull,
                extensionListArbEmpty,
                foundExt,
                extensionListExtNull,
                extensionListExtEmpty,
                isRemoteSession);

            return false;
        }

        // Load each extension individually.
#define LOAD_WGL_EXTENSION_FUNCTION(return_type, function_name, signature) \
    this->function_name = reinterpret_cast< \
            return_type (GL_APIENTRY*) signature>( \
                    this->findFunction(#function_name "ARB")); \
    if (!this->function_name) { \
        this->function_name = reinterpret_cast< \
                return_type (GL_APIENTRY*) signature>( \
                        this->findFunction(#function_name "EXT")); \
    } \
    if (!this->function_name) { \
        WGL_ERR("ERROR: %s: Missing extension function %s\n", __FUNCTION__, \
            #function_name); \
        result = false; \
    }

#define LOAD_WGL_EXTENSION(extension) \
    if (supportsExtension("WGL_ARB_" #extension, extensionList) || \
        supportsExtension("WGL_EXT_" #extension, extensionList)) { \
        LIST_##extension##_FUNCTIONS(LOAD_WGL_EXTENSION_FUNCTION) \
    } else { \
        WGL_ERR("WARNING: %s: Missing WGL extension %s\n", __FUNCTION__, #extension); \
    }

        LOAD_WGL_EXTENSION(pixel_format)
        LOAD_WGL_EXTENSION(make_current_read)
        LOAD_WGL_EXTENSION(pbuffer)
        LOAD_WGL_EXTENSION(create_context)
        LOAD_WGL_EXTENSION(swap_control)

        // Done.
        return result;
    }

private:
    WglExtensionsDispatch();  // no default constructor.
};

const WglExtensionsDispatch* initExtensionsDispatch(
        const WglBaseDispatch* dispatch) {
    HWND hwnd = createDummyWindow();
    HDC hdc =  GetDC(hwnd);
    if (!hwnd || !hdc){
        int err = GetLastError();
        WGL_ERR("error while getting DC: 0x%x\n", err);
        return NULL;
    }
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),  //  size of this pfd
        1,                     // version number
        PFD_DRAW_TO_WINDOW |   // support window
        PFD_SUPPORT_OPENGL |   // support OpenGL
        PFD_DOUBLEBUFFER,      // double buffered
        PFD_TYPE_RGBA,         // RGBA type
        32,                    // 32-bit color depth
        0, 0, 0, 0, 0, 0,      // color bits ignored
        0,                     // no alpha buffer
        0,                     // shift bit ignored
        0,                     // no accumulation buffer
        0, 0, 0, 0,            // accum bits ignored
        24,                    // 24-bit z-buffer
        0,                     // no stencil buffer
        0,                     // no auxiliary buffer
        PFD_MAIN_PLANE,        // main layer
        0,                     // reserved
        0, 0, 0                // layer masks ignored
    };

    int iPixelFormat = dispatch->ChoosePixelFormat(hdc, &pfd);
    if (iPixelFormat <= 0) {
        int err = GetLastError();
        WGL_ERR("error while choosing pixel format 0x%x\n", err);
        return NULL;
    }
    if (!dispatch->SetPixelFormat(hdc, iPixelFormat, &pfd)) {
        int err = GetLastError();
        WGL_ERR("error while setting pixel format 0x%x\n", err);
        return NULL;
    }

    int err;
    HGLRC ctx = dispatch->wglCreateContext(hdc);
    if (!ctx) {
        err =  GetLastError();
        WGL_ERR("error while creating dummy context: 0x%x\n", err);
    }
    if (!dispatch->wglMakeCurrent(hdc, ctx)) {
        err =  GetLastError();
        WGL_ERR("error while making dummy context current: 0x%x\n", err);
    }

    WglExtensionsDispatch* result = new WglExtensionsDispatch(*dispatch);
    result->init(hdc);

    dispatch->wglMakeCurrent(hdc, NULL);
    dispatch->wglDeleteContext(ctx);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    return result;
}

class WinPixelFormat : public EglOS::PixelFormat {
public:
    WinPixelFormat(const PIXELFORMATDESCRIPTOR* desc, int configId)
        : mDesc(*desc), mConfigId(configId) {}

    EglOS::PixelFormat* clone() {
        return new WinPixelFormat(&mDesc, mConfigId);
    }

    const PIXELFORMATDESCRIPTOR* desc() const { return &mDesc; }
    const int configId() const { return mConfigId; }

    static const WinPixelFormat* from(const EglOS::PixelFormat* f) {
        return static_cast<const WinPixelFormat*>(f);
    }

private:
    WinPixelFormat(const WinPixelFormat& other) = delete;

    PIXELFORMATDESCRIPTOR mDesc = {};
    int mConfigId = 0;
};

class WinSurface : public EglOS::Surface {
public:
    explicit WinSurface(HWND wnd, const WglExtensionsDispatch* dispatch) :
            Surface(WINDOW),
            m_hwnd(wnd),
            m_hdc(GetDC(wnd)),
            m_dispatch(dispatch) {}

    explicit WinSurface(HPBUFFERARB pb, const EglOS::PixelFormat* pixelFormat, const WglExtensionsDispatch* dispatch) :
            Surface(PBUFFER),
            m_pb(pb),
            m_pixelFormat(pixelFormat),
            m_dispatch(dispatch) {
        refreshDC();
    }

    bool releaseDC() {
        if (m_dcReleased) return true;

        bool res = false;
        if (m_dispatch->wglReleasePbufferDC) {
            res = m_dispatch->wglReleasePbufferDC(m_pb, m_hdc);
            m_hdc = 0;
            m_dcReleased = true;
        }
        return res;
    }

    bool refreshDC() {
        if (!m_dcReleased) releaseDC();
        if (m_dispatch->wglGetPbufferDC) {
            m_hdc = m_dispatch->wglGetPbufferDC(m_pb);
        }

        if (!m_hdc) return false;

        m_dcReleased = false;

        return true;
    }

    ~WinSurface() {
        if (type() == WINDOW) {
            ReleaseDC(m_hwnd, m_hdc);
        }
    }

    void onMakeCurrent() {
        if (m_everMadeCurrent) return;
        if (m_dispatch->wglSwapInterval) {
            m_dispatch->wglSwapInterval(0);
        }
        m_everMadeCurrent = true;
    }

    HWND getHwnd() const { return m_hwnd; }
    HDC  getDC() const { return m_hdc; }
    HPBUFFERARB getPbuffer() const { return m_pb; }
    const EglOS::PixelFormat* getPixelFormat() const { return m_pixelFormat; }

    static WinSurface* from(EglOS::Surface* s) {
        return static_cast<WinSurface*>(s);
    }

private:
    bool        m_everMadeCurrent = false;
    bool        m_dcReleased = true;
    HWND        m_hwnd = nullptr;
    HPBUFFERARB m_pb = nullptr;
    HDC         m_hdc = nullptr;
    const EglOS::PixelFormat* m_pixelFormat = nullptr;
    const WglExtensionsDispatch* m_dispatch = nullptr;
};

static android::base::StaticLock sGlobalLock;

class WinContext : public EglOS::Context {
public:
    explicit WinContext(const WglExtensionsDispatch* dispatch, HGLRC ctx) :
        mDispatch(dispatch), mCtx(ctx) {}

    ~WinContext() {
        android::base::AutoLock lock(sGlobalLock);
        if (!mDispatch->wglDeleteContext(mCtx)) {
            WGL_ERR("error deleting WGL context! error 0x%x\n",
                    (unsigned)GetLastError());
        }
    }

    HGLRC context() const { return mCtx; }

    static HGLRC from(const EglOS::Context* c) {
        if (!c) return nullptr;
        return static_cast<const WinContext*>(c)->context();
    }

private:
    const WglExtensionsDispatch* mDispatch = nullptr;
    HGLRC mCtx = nullptr;
};

// A helper class used to deal with a vexing limitation of the WGL API.
// The documentation for SetPixelFormat() states the following:
//
// -- If hdc references a window, calling the SetPixelFormat function also
// -- changes the pixel format of the window. Setting the pixel format of a
// -- window more than once [...] is not allowed. An application can only set
// -- the pixel format of a window one time. Once a window's pixel format is
// -- set, it cannot be changed.
// --
// -- You should select a pixel format in the device context before calling
// -- the wglCreateContext function. The wglCreateContext function creates a
// -- rendering context for drawing on the device in the selected pixel format
// -- of the device context.
//
// In other words, creating a GL context requires having a unique window and
// device context for the corresponding EGLConfig.
//
// This code deals with this by implementing the followin scheme:
//
// - For each unique PixelFormat ID (a.k.a. EGLConfig number), provide a way
//   to create a new hidden 1x1 window, and corresponding HDC.
//
// - Implement a simple thread-local mapping from PixelFormat IDs to
//   (HWND, HDC) pairs that are created on demand.
//
// WinGlobals is the class used to implement this scheme. Usage is the
// following:
//
// - Create a single global instance, passing a WglBaseDispatch pointer
//   which is required to call its SetPixelFormat() method.
//
// - Call getDefaultDummyDC() to retrieve a thread-local device context that
//   can be used to query / probe the list of available pixel formats for the
//   host window, but not perform rendering.
//
// - Call getDummyDC() to retrieve a thread-local device context that can be
//   used to create WGL context objects to render into a specific pixel
//   format.
//
// - These devic contexts are thread-local, i.e. they are automatically
//   reclaimed on thread exit. This also means that the caller should not
//   call either ReleaseDC() or DeleteDC() on them.
//
class WinGlobals {
public:
    // Constructor. |dispatch| will be used to call the right version of
    // ::SetPixelFormat() depending on GPU emulation configuration. See
    // technical notes above for details.
    explicit WinGlobals(const WglBaseDispatch* dispatch)
            : mDispatch(dispatch), mTls(onThreadTermination) {}

    // Return a thread-local device context that can be used to list
    // available pixel formats for the host window. The caller cannot use
    // this context for drawing though. The context is owned
    // by this instance and will be automatically reclaimed on thread exit.
    HDC getDefaultDummyDC() {
        return getInternalDC(nullptr);
    }

    HDC getDefaultNontrivialDC() {
        return mNontrivialDC;
    }
    void setNontrivialDC(const WinPixelFormat* format) {
        mNontrivialDC = getInternalDC(format);
    }

    // Return a thread-local device context associated with a specific
    // pixel format. The result is owned by this instance, and is automatically
    // reclaimed on thread exit. Don't try to call ReleaseDC() on it.
    HDC getDummyDC(const WinPixelFormat* format) {
        return getInternalDC(format);
    }

private:
    // ConfigDC holds a (HWND,HDC) pair to be associated with a given
    // pixel format ID. This serves as the value type for ConfigMap
    // declared below. Implemented as a movable type without copy-operations.
    class ConfigDC {
    public:
        // Constructor. This creates a new dummy 1x1 invisible window and
        // an associated device context. If |format| is not nullptr, then
        // calls |dispatch->SetPixelFormat()| on the resulting context to
        // set the window's pixel format.
        ConfigDC(const WinPixelFormat* format,
                 const WglBaseDispatch* dispatch) {
            mWindow = createDummyWindow();
            if (mWindow) {
                mDeviceContext = GetDC(mWindow);
                if (format) {
                    dispatch->SetPixelFormat(mDeviceContext,
                                             format->configId(),
                                             format->desc());
                }
            }
        }

        // Destructor.
        ~ConfigDC() {
            if (mWindow) {
                ReleaseDC(mWindow, mDeviceContext);
                DestroyWindow(mWindow);
                mWindow = nullptr;
            }
        }

        // Supports moves - this disables auto-generated copy-constructors.
        ConfigDC(ConfigDC&& other)
                : mWindow(other.mWindow),
                  mDeviceContext(other.mDeviceContext) {
            other.mWindow = nullptr;
        }

        ConfigDC& operator=(ConfigDC&& other) {
            mWindow = other.mWindow;
            mDeviceContext = other.mDeviceContext;
            other.mWindow = nullptr;
            return *this;
        }

        // Return device context for this instance.
        HDC dc() const { return mDeviceContext; }

    private:
        HWND mWindow = nullptr;
        HDC mDeviceContext = nullptr;
    };

    // Convenience type alias for mapping pixel format IDs, a.k.a. EGLConfig
    // IDs, to ConfigDC instances.
    using ConfigMap = std::unordered_map<int, ConfigDC>;

    // Called when a thread terminates to delete the thread-local map
    // that associates pixel format IDs with ConfigDC instances.
    static void onThreadTermination(void* opaque) {
        auto map = reinterpret_cast<ConfigMap*>(opaque);
        delete map;
    }

    // Helper function used by getDefaultDummyDC() and getDummyDC().
    //
    // If |format| is nullptr, return a thread-local device context that can
    // be used to list all available pixel formats for the host window.
    //
    // If |format| is not nullptr, then return a thread-local device context
    // associated with the corresponding pixel format.
    //
    // Both cases will lazily create a 1x1 invisible dummy window which
    // the device is connected to. All objects are automatically destroyed
    // on thread exit. Return nullptr on failure.
    HDC getInternalDC(const WinPixelFormat* format) {
        int formatId = format ? format->configId() : 0;
        auto map = reinterpret_cast<ConfigMap*>(mTls.get());
        if (!map) {
            map = new ConfigMap();
            mTls.set(map);
        }

        auto it = map->find(formatId);
        if (it != map->end()) {
            return it->second.dc();
        }

        ConfigDC newValue(format, mDispatch);
        HDC result = newValue.dc();
        map->emplace(formatId, std::move(newValue));
        return result;
    }

    const WglBaseDispatch* mDispatch = nullptr;
    emugl::ThreadStore mTls;
    HDC mNontrivialDC = nullptr;
};

bool initPixelFormat(HDC dc, const WglExtensionsDispatch* dispatch) {
    if (dispatch->wglChoosePixelFormat) {
        unsigned int numpf;
        int iPixelFormat;
        int i0 = 0;
        float f0 = 0.0f;
        return dispatch->wglChoosePixelFormat(
                dc, &i0, &f0, 1, &iPixelFormat, &numpf);
    } else {
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),  //  size of this pfd
            1,                     // version number
            PFD_DRAW_TO_WINDOW |   // support window
            PFD_SUPPORT_OPENGL |   // support OpenGL
            PFD_DOUBLEBUFFER,      // double buffered
            PFD_TYPE_RGBA,         // RGBA type
            32,                    // 32-bit color depth
            0, 0, 0, 0, 0, 0,      // color bits ignored
            0,                     // no alpha buffer
            0,                     // shift bit ignored
            0,                     // no accumulation buffer
            0, 0, 0, 0,            // accum bits ignored
            24,                    // 24-bit z-buffer
            0,                     // no stencil buffer
            0,                     // no auxiliary buffer
            PFD_MAIN_PLANE,        // main layer
            0,                     // reserved
            0, 0, 0                // layer masks ignored
        };
        return dispatch->ChoosePixelFormat(dc, &pfd);
    }
}

void pixelFormatToConfig(WinGlobals* globals,
                         const WglExtensionsDispatch* dispatch,
                         int renderableType,
                         const PIXELFORMATDESCRIPTOR* frmt,
                         int index,
                         EglOS::AddConfigCallback* addConfigFunc,
                         void* addConfigOpaque) {
    EglOS::ConfigInfo info;
    memset(&info, 0, sizeof(info));

    if (frmt->iPixelType != PFD_TYPE_RGBA) {
        D("%s: Not an RGBA type!\n", __FUNCTION__);
        return; // other formats are not supported yet
    }
    if (!(frmt->dwFlags & PFD_SUPPORT_OPENGL)) {
        D("%s: No OpenGL support\n", __FUNCTION__);
        return;
    }
    // NOTE: Software renderers don't always support double-buffering.
    if (dispatch->mIsSystemLib && !(frmt->dwFlags & PFD_DOUBLEBUFFER)) {
        D("%s: No double-buffer support\n", __FUNCTION__);
        return;
    }
    if ((frmt->dwFlags & (PFD_GENERIC_FORMAT | PFD_NEED_PALETTE)) != 0) {
        //discard generic pixel formats as well as pallete pixel formats
        D("%s: Generic format or needs palette\n", __FUNCTION__);
        return;
    }

    if (!dispatch->wglGetPixelFormatAttribiv) {
        D("%s: Missing wglGetPixelFormatAttribiv\n", __FUNCTION__);
        return;
    }

    GLint window = 0, pbuffer = 0;
    HDC dpy = globals->getDefaultDummyDC();

    if (dispatch->mIsSystemLib) {
        int windowAttrib = WGL_DRAW_TO_WINDOW_ARB;
        EXIT_IF_FALSE(dispatch->wglGetPixelFormatAttribiv(
                dpy, index, 0, 1, &windowAttrib, &window));
    }
    int pbufferAttrib = WGL_DRAW_TO_PBUFFER_ARB;
    EXIT_IF_FALSE(dispatch->wglGetPixelFormatAttribiv(
            dpy, index, 0, 1, &pbufferAttrib, &pbuffer));

    info.surface_type = 0;
    if (window) {
        info.surface_type |= EGL_WINDOW_BIT;
    }
    if (pbuffer) {
        info.surface_type |= EGL_PBUFFER_BIT;
    }
    if (!info.surface_type) {
        D("%s: Missing surface type\n", __FUNCTION__);
        return;
    }

    //default values
    info.native_visual_id = 0;
    info.native_visual_type = EGL_NONE;
    info.caveat = EGL_NONE;
    info.native_renderable = EGL_FALSE;
    info.renderable_type = renderableType;
    info.max_pbuffer_width = PBUFFER_MAX_WIDTH;
    info.max_pbuffer_height = PBUFFER_MAX_HEIGHT;
    info.max_pbuffer_size = PBUFFER_MAX_PIXELS;
    info.samples_per_pixel = 0;
    info.frame_buffer_level = 0;

    GLint transparent;
    int transparentAttrib = WGL_TRANSPARENT_ARB;
    EXIT_IF_FALSE(dispatch->wglGetPixelFormatAttribiv(
            dpy, index, 0, 1, &transparentAttrib, &transparent));
    if (transparent) {
        info.transparent_type = EGL_TRANSPARENT_RGB;
        int transparentRedAttrib = WGL_TRANSPARENT_RED_VALUE_ARB;
        EXIT_IF_FALSE(dispatch->wglGetPixelFormatAttribiv(
                dpy, index, 0, 1, &transparentRedAttrib, &info.trans_red_val));
        int transparentGreenAttrib = WGL_TRANSPARENT_GREEN_VALUE_ARB;
        EXIT_IF_FALSE(dispatch->wglGetPixelFormatAttribiv(
                dpy, index, 0, 1, &transparentGreenAttrib,
                &info.trans_green_val));
        int transparentBlueAttrib = WGL_TRANSPARENT_RED_VALUE_ARB;
        EXIT_IF_FALSE(dispatch->wglGetPixelFormatAttribiv(
                dpy,index, 0, 1, &transparentBlueAttrib, &info.trans_blue_val));
    } else {
        info.transparent_type = EGL_NONE;
    }

    info.red_size = frmt->cRedBits;
    info.green_size = frmt->cGreenBits;
    info.blue_size = frmt->cBlueBits;
    info.alpha_size = frmt->cAlphaBits;
    info.depth_size = frmt->cDepthBits;
    info.stencil_size = frmt->cStencilBits;

    info.frmt = new WinPixelFormat(frmt, index);

    if (!globals->getDefaultNontrivialDC() &&
        info.red_size >= 8) {
        globals->setNontrivialDC(WinPixelFormat::from(info.frmt));
    }

    (*addConfigFunc)(addConfigOpaque, &info);
}

class WglDisplay : public EglOS::Display {
public:
    WglDisplay(const WglExtensionsDispatch* dispatch,
               WinGlobals* globals)
            : mDispatch(dispatch), mGlobals(globals) {}

    virtual EglOS::GlesVersion getMaxGlesVersion() {
        if (!mCoreProfileSupported) {
            return EglOS::GlesVersion::ES2;
        }

        return EglOS::calcMaxESVersionFromCoreVersion(
                   mCoreMajorVersion, mCoreMinorVersion);
    }

    virtual void queryConfigs(int renderableType,
                              EglOS::AddConfigCallback addConfigFunc,
                              void* addConfigOpaque) {
        HDC dpy = mGlobals->getDefaultDummyDC();

        // wglChoosePixelFormat() needs to be called at least once,
        // i.e. it seems that the driver needs to initialize itself.
        // Do it here during initialization.
        initPixelFormat(dpy, mDispatch);

        // Quering number of formats
        PIXELFORMATDESCRIPTOR  pfd;
        int maxFormat = mDispatch->DescribePixelFormat(
                dpy, 1, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        if (0 == maxFormat) {
            WGL_ERR("No pixel formats found from wglDescribePixelFormat! "
                    "error: 0x%x\n", GetLastError());
        }

        // Inserting rest of formats. Try to map each one to an EGL Config.
        for (int configId = 1; configId <= maxFormat; configId++) {
            mDispatch->DescribePixelFormat(
                    dpy, configId, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
            pixelFormatToConfig(
                    mGlobals,
                    mDispatch,
                    renderableType,
                    &pfd,
                    configId,
                    addConfigFunc,
                    addConfigOpaque);
        }

        queryCoreProfileSupport();

        if (mDispatch->wglSwapInterval) {
            mDispatch->wglSwapInterval(0); // use guest SF / HWC instead
        }
    }

    virtual bool isValidNativeWin(EglOS::Surface* win) {
        if (!win) {
            return false;
        } else {
            return isValidNativeWin(WinSurface::from(win)->getHwnd());
        }
    }

    virtual bool isValidNativeWin(EGLNativeWindowType win) {
        return IsWindow(win);
    }

    virtual bool checkWindowPixelFormatMatch(
            EGLNativeWindowType win,
            const EglOS::PixelFormat* pixelFormat,
            unsigned int* width,
            unsigned int* height) {
        RECT r;
        if (!GetClientRect(win, &r)) {
            return false;
        }
        *width  = r.right  - r.left;
        *height = r.bottom - r.top;
        HDC dc = GetDC(win);
        const WinPixelFormat* format = WinPixelFormat::from(pixelFormat);
        bool ret = mDispatch->SetPixelFormat(dc,
                                             format->configId(),
                                             format->desc());
        ReleaseDC(win, dc);
        return ret;
    }

    virtual emugl::SmartPtr<EglOS::Context> createContext(
            EGLint profileMask,
            const EglOS::PixelFormat* pixelFormat,
            EglOS::Context* sharedContext) {

        android::base::AutoLock lock(sGlobalLock);

        const WinPixelFormat* format = WinPixelFormat::from(pixelFormat);
        HDC dpy = mGlobals->getDummyDC(format);
        if (!dpy) {
            return nullptr;
        }

        bool useCoreProfile =
            mCoreProfileSupported &&
            (profileMask & EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR);

        HGLRC ctx;
        if (useCoreProfile) {
            ctx = mDispatch->wglCreateContextAttribs(
                      dpy, WinContext::from(sharedContext),
                      mCoreProfileCtxAttribs);
        } else {
            ctx = mDispatch->wglCreateContext(dpy);
            if (ctx && sharedContext) {
                if (!mDispatch->wglShareLists(WinContext::from(sharedContext), ctx)) {
                    mDispatch->wglDeleteContext(ctx);
                    return NULL;
                }
            }
        }

        return std::make_shared<WinContext>(mDispatch, ctx);
    }

    virtual EglOS::Surface* createPbufferSurface(
            const EglOS::PixelFormat* pixelFormat,
            const EglOS::PbufferInfo* info) {
        android::base::AutoLock lock(sGlobalLock);
        (void)info;

        bool needPrime = false;

        auto& freeElts = mFreePbufs[pixelFormat];

        if (freeElts.empty()) {
            needPrime = true;
        }

        if (needPrime) {
            // Policy here is based on the behavior of Android UI when opening
            // and closing lots of apps, and to avoid frequent pauses to create
            // pbuffers.  We start by priming with 8 pbuffers, and then double
            // the current set of live pbuffers each time. This will then
            // require only a few primings (up to ~32 pbuffers) to make it
            // unnecessary to create or delete a pbuffer ever again after that.
            // Creating many pbuffers in a batch is faster than interrupting
            // the process at various times to create them one at a time.
            PROFILE_SLOW("createPbufferSurface (slow path)");
            int toCreate = std::max((int)mLivePbufs[pixelFormat].size(), kPbufPrimingCount);
            for (int i = 0; i < toCreate; i++) {
                freeElts.push_back(createPbufferSurfaceImpl(pixelFormat));
            }
        }

        PROFILE_SLOW("createPbufferSurface (fast path)");
        EglOS::Surface* surf = freeElts.back();
        WinSurface* winSurface = WinSurface::from(surf);
        if (!winSurface->refreshDC()) {
            // TODO: Figure out why some Intel GPUs can hang flakily if
            // we just destroy the pbuffer here.
            mDeletePbufs.push_back(surf);
            surf = createPbufferSurfaceImpl(pixelFormat);
        }
        freeElts.pop_back();

        mLivePbufs[pixelFormat].insert(surf);

        return surf;
    }

    virtual bool releasePbuffer(EglOS::Surface* pb) {
        android::base::AutoLock lock(sGlobalLock);
        if (!pb) return false;

        WinSurface* winpb = WinSurface::from(pb);

        if (!winpb->releaseDC()) return false;

        const EglOS::PixelFormat* pixelFormat =
            winpb->getPixelFormat();

        auto& frees = mFreePbufs[pixelFormat];
        frees.push_back(pb);

        mLivePbufs[pixelFormat].erase(pb);

        for (auto surf : mDeletePbufs) {
            WinSurface* winSurface = WinSurface::from(surf);
            mDispatch->wglDestroyPbuffer(winSurface->getPbuffer());
            delete winSurface;
        }

        mDeletePbufs.clear();

        return true;
    }

    virtual bool makeCurrent(EglOS::Surface* read,
                             EglOS::Surface* draw,
                             EglOS::Context* context) {
        android::base::AutoLock lock(sGlobalLock);
        WinSurface* readWinSurface = WinSurface::from(read);
        WinSurface* drawWinSurface = WinSurface::from(draw);
        HDC hdcRead = read ? readWinSurface->getDC() : NULL;
        HDC hdcDraw = draw ? drawWinSurface->getDC() : NULL;
        HGLRC hdcContext = context ? WinContext::from(context) : 0;

        const WglExtensionsDispatch* dispatch = mDispatch;

        bool ret = false;
        if (hdcRead == hdcDraw){
            // The following loop is a work-around for a problem when
            // occasionally the rendering is incorrect after hibernating and
            // waking up windows.
            // wglMakeCurrent will sometimes fail for a short period of time
            // in such situation.
            //
            // For a stricter test, in addition to checking the return value, we
            // might also want to check its error code. On my computer in such
            // situation GetLastError() returns 0 (which is documented as
            // success code). This is not a documented behaviour and is
            // unreliable. But in case one needs to use it, here is the code:
            //
            //      while (!(isSuccess = dispatch->wglMakeCurrent(hdcDraw, hdcContext))
            //           && GetLastError()==0) Sleep(16);
            //

            int count = 100;
            while (!dispatch->wglMakeCurrent(hdcDraw, hdcContext)
                   && --count > 0
                   && !GetLastError()) {
                Sleep(16);
            }
            if (count <= 0) {
                D("Error: wglMakeCurrent() failed, error %d\n", (int)GetLastError());
                return false;
            }
            ret = true;
        } else if (!dispatch->wglMakeContextCurrent) {
            return false;
        } else {
            ret = dispatch->wglMakeContextCurrent(
                    hdcDraw, hdcRead, hdcContext);
        }

        if (ret) {
            if (readWinSurface) readWinSurface->onMakeCurrent();
            if (drawWinSurface) drawWinSurface->onMakeCurrent();
        }
        return ret;
    }

    virtual void swapBuffers(EglOS::Surface* srfc) {
        android::base::AutoLock lock(sGlobalLock);
        if (srfc && !mDispatch->SwapBuffers(WinSurface::from(srfc)->getDC())) {
            GetLastError();
        }
    }

private:
    // Returns the highest level of OpenGL core profile support in
    // this WGL implementation.
    void queryCoreProfileSupport() {
        mCoreProfileSupported = false;

        if (!mDispatch->wglCreateContextAttribs) {
            WGL_ERR("OpenGL Core Profile not supported.\n");
            // Not supported, don't even try.
            return;
        }

        // Ascending index order of context attribs :
        // decreasing GL major/minor version
        HGLRC testContext = nullptr;
        HDC dpy = mGlobals->getDefaultNontrivialDC();

        for (int i = 0; i < getNumCoreProfileCtxAttribs(); i++) {
            const int* attribs = getCoreProfileCtxAttribs(i);
            testContext =
                mDispatch->wglCreateContextAttribs(
                        dpy, nullptr /* no shared context */,
                        attribs);

            if (testContext) {
                mCoreProfileSupported = true;
                mCoreProfileCtxAttribs = attribs;
                getCoreProfileCtxAttribsVersion(
                    attribs, &mCoreMajorVersion, &mCoreMinorVersion);
                mDispatch->wglDeleteContext(testContext);
                return;
            }
        }
    }

    EglOS::Surface* createPbufferSurfaceImpl(const EglOS::PixelFormat* pixelFormat) {
        // we never care about width or height, since we just use
        // opengl fbos anyway.
        const WinPixelFormat* format = WinPixelFormat::from(pixelFormat);
        HDC dpy = mGlobals->getDummyDC(format);

        const WglExtensionsDispatch* dispatch = mDispatch;
        if (!dispatch->wglCreatePbuffer) {
            return NULL;
        }
        HPBUFFERARB pb = dispatch->wglCreatePbuffer(dpy, format->configId(), 1, 1, nullptr);
        if (!pb) {
            GetLastError();
            return NULL;
        }
        return new WinSurface(pb, pixelFormat, dispatch);
    }

    bool mCoreProfileSupported = false;
    int mCoreMajorVersion = 4;
    int mCoreMinorVersion = 5;
    const int* mCoreProfileCtxAttribs = nullptr;
    WinPixelFormat* mDefaultPixelFormat = nullptr;

    const WglExtensionsDispatch* mDispatch = nullptr;
    WinGlobals* mGlobals = nullptr;

    std::unordered_map<const EglOS::PixelFormat*, std::vector<EglOS::Surface* > > mFreePbufs;
    std::unordered_map<const EglOS::PixelFormat*, std::unordered_set<EglOS::Surface* > > mLivePbufs;
    std::vector<EglOS::Surface*> mDeletePbufs;
    static constexpr int kPbufPrimingCount = 8;
};

constexpr int WglDisplay::kPbufPrimingCount;

class WglLibrary : public GlLibrary {
public:
    WglLibrary(const WglBaseDispatch* dispatch) : mDispatch(dispatch) {}

    virtual GlFunctionPointer findSymbol(const char* name) {
        return mDispatch->findFunction(name);
    }

private:
    const WglBaseDispatch* mDispatch = nullptr;
};

class WinEngine : public EglOS::Engine {
public:
    WinEngine();

    ~WinEngine() {
        delete mDispatch;
    }

    virtual EglOS::Display* getDefaultDisplay() {
        return new WglDisplay(mDispatch, &mGlobals);
    }

    virtual GlLibrary* getGlLibrary() {
        return &mGlLib;
    }

    virtual EglOS::Surface* createWindowSurface(EglOS::PixelFormat* cfg,
                                                EGLNativeWindowType wnd) {
        (void)cfg;
        return new WinSurface(wnd, mDispatch);
    }

private:
    SharedLibrary* mLib = nullptr;
    const WglExtensionsDispatch* mDispatch = nullptr;
    WglBaseDispatch mBaseDispatch = {};
    WglLibrary mGlLib;
    WinGlobals mGlobals;
};

WinEngine::WinEngine() :
        mGlLib(&mBaseDispatch),
        mGlobals(&mBaseDispatch) {
    const char* kLibName = "opengl32.dll";
    bool isSystemLib = true;
    const char* env = ::getenv("ANDROID_GL_LIB");
    if (env && !strcmp(env, "mesa")) {
        kLibName = "mesa_opengl32.dll";
        isSystemLib = false;
    }
    char error[256];
    GL_LOG("%s: Trying to load %s\n", __FUNCTION__, kLibName);
    mLib = SharedLibrary::open(kLibName, error, sizeof(error));
    if (!mLib) {
        WGL_ERR("ERROR: %s: Could not open %s: %s\n", __FUNCTION__,
                kLibName, error);
        exit(1);
    }

    GL_LOG("%s: Library loaded at %p\n", __FUNCTION__, mLib);
    mBaseDispatch.init(mLib, isSystemLib);
    mDispatch = initExtensionsDispatch(&mBaseDispatch);
    GL_LOG("%s: Dispatch initialized\n", __FUNCTION__);
}

emugl::LazyInstance<WinEngine> sHostEngine = LAZY_INSTANCE_INIT;

}  // namespace

// static
EglOS::Engine* EglOS::Engine::getHostInstance() {
    return sHostEngine.ptr();
}
