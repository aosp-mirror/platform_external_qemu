/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Android Emulator platform.
 *//*--------------------------------------------------------------------*/

#include "tcuAEMUPlatform.hpp"
#include "tcuFunctionLibrary.hpp"
#include "egluDefs.hpp"
#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluGLContextFactory.hpp"
#include "eglwEnums.hpp"
#include "eglwLibrary.hpp"
#include "deMemory.h"

// AEMU graphics include
#include "Toplevel.h"

// Name of the android emulator EGL library
#ifdef __APPLE__
#define GOLDFISH_OPENGL_EGL_LIBRARY "libEGL_emulation.dylib"
#else
#define GOLDFISH_OPENGL_EGL_LIBRARY "libEGL_emulation.so"
#endif

tcu::Platform* createPlatform (void)
{
	return new tcu::AEMUPlatform();
}

namespace tcu
{

enum
{
	DEFAULT_WINDOW_WIDTH	= 256,
	DEFAULT_WINDOW_HEIGHT	= 256
};

static const eglu::NativeDisplay::Capability	DISPLAY_CAPABILITIES	= eglu::NativeDisplay::CAPABILITY_GET_DISPLAY_LEGACY;
static const eglu::NativeWindow::Capability		WINDOW_CAPABILITIES		= eglu::NativeWindow::CAPABILITY_CREATE_SURFACE_LEGACY;

static aemu::Toplevel* sToplevel = nullptr;

static aemu::Toplevel* sInitToplevel() {
	if (sToplevel) return sToplevel;
	sToplevel = new aemu::Toplevel(10000 /* allow high refresh rate */);
	return sToplevel;
}

class Display : public eglu::NativeDisplay
{
public:
	Display(void) : eglu::NativeDisplay(DISPLAY_CAPABILITIES), m_toplevel(sInitToplevel()), m_library(GOLDFISH_OPENGL_EGL_LIBRARY) {}
	~Display(void) {}

	virtual eglw::EGLNativeDisplayType getLegacyNative(void) { return EGL_DEFAULT_DISPLAY; }
	virtual const eglw::Library &getLibrary(void) const
	{
		return m_library;
	}

	aemu::Toplevel *toplevel() { return m_toplevel; }

	aemu::Toplevel *m_toplevel;
	eglw::DefaultLibrary m_library;
};

class DisplayFactory : public eglu::NativeDisplayFactory
{
  public:
	DisplayFactory(void);
	~DisplayFactory(void) {}

	eglu::NativeDisplay *createDisplay(const eglw::EGLAttrib *attribList) const;
};

class Window : public eglu::NativeWindow
{
public:
	Window(int width, int height) : eglu::NativeWindow(WINDOW_CAPABILITIES), m_width(width), m_height(height)
	{
		m_nativeWindow = sToplevel->createWindow();
	}

	~Window(void) { sToplevel->destroyWindow(m_nativeWindow); }

	eglw::EGLNativeWindowType getLegacyNative(void) { return m_nativeWindow; }
	IVec2 getSize(void) const { return IVec2(m_width, m_height); }

private:
	int m_width;
	int m_height;
	void *m_nativeWindow;
};

class WindowFactory : public eglu::NativeWindowFactory
{
  public:
	WindowFactory(void) : eglu::NativeWindowFactory("AEMU", "Android Emulator Window", WINDOW_CAPABILITIES) {}
	virtual ~WindowFactory(void) {}

	virtual eglu::NativeWindow *createWindow(eglu::NativeDisplay *nativeDisplay, const eglu::WindowParams &params) const
	{
		(void)nativeDisplay;
		const int width = params.width != eglu::WindowParams::SIZE_DONT_CARE ? params.width : DEFAULT_WINDOW_WIDTH;
		const int height = params.height != eglu::WindowParams::SIZE_DONT_CARE ? params.height : DEFAULT_WINDOW_HEIGHT;
		return new Window(width, height);
	}
};

// DisplayFactory

DisplayFactory::DisplayFactory(void)
	: eglu::NativeDisplayFactory("default", "EGL_DEFAULT_DISPLAY", DISPLAY_CAPABILITIES)
{
	m_nativeWindowRegistry.registerFactory(new WindowFactory());
}

eglu::NativeDisplay *DisplayFactory::createDisplay(const eglw::EGLAttrib *) const
{
	return new Display();
}

// AEMUPlatform

AEMUPlatform::AEMUPlatform(void)
{
	m_nativeDisplayFactoryRegistry.registerFactory(new DisplayFactory());
	m_contextFactoryRegistry.registerFactory(new eglu::GLContextFactory(m_nativeDisplayFactoryRegistry));
}

AEMUPlatform::~AEMUPlatform(void) { }

} // namespace tcu
