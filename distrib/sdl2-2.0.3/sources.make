# This is included from the main Android emulator build script
# to declare the SDL-related sources, compiler flags and libraries
#

SDL2_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

SDL2_INCLUDES := $(LOCAL_PATH)/config/include $(LOCAL_PATH)/include
SDL2_CFLAGS :=
SDL2_LDLIBS :=
SDL2_SOURCES :=

SDL2_CFLAGS += -DSDL_AUDIO_DISABLED=1

ifeq ($(HOST_OS),linux)
    SDL2_CONFIG_LOADSO_DLOPEN := yes
    SDL2_CONFIG_THREAD_PTHREAD := yes
    SDL2_CONFIG_TIMER_UNIX := yes
    SDL2_CONFIG_VIDEO_X11 := yes
    SDL2_CONFIG_RENDER_OPENGL := yes
    SDL2_CONFIG_JOYSTICK_LINUX := yes
    SDL2_CONFIG_MAIN_DUMMY := yes

    SDL2_CFLAGS += -D_REENTRANT
    SDL2_LDLIBS += -lm -ldl -lpthread -lrt
endif

ifeq ($(HOST_OS),darwin)
    SDL2_CONFIG_LOADSO_DLOPEN := yes
    SDL2_CONFIG_THREAD_PTHREAD := yes
    SDL2_CONFIG_TIMER_UNIX := yes
    SDL2_CONFIG_VIDEO_COCOA := yes
    SDL2_CONFIG_RENDER_OPENGL := yes
    SDL2_CONFIG_JOYSTICK_DARWIN := yes
    SDL2_CONFIG_MAIN_DUMMY := yes

    SDL2_CFLAGS += -DTHREAD_SAFE
    FRAMEWORKS := OpenGL Cocoa ApplicationServices Carbon IOKit
    SDL2_LDLIBS += $(FRAMEWORKS:%=-Wl,-framework,%)

    # SDK 10.6+ deprecates __dyld_func_lookup required by dlcompat_init_func
    # in SDL_dlcompat.o this module depends.  Instruct linker to resolve it
    # at runtime.
#     OSX_VERSION_MAJOR := $(shell echo $(mac_sdk_version) | cut -d . -f 2)
#     OSX_VERSION_MAJOR_GREATER_THAN_OR_EQUAL_TO_6 := $(shell [ $(OSX_VERSION_MAJOR) -ge 6 ] && echo true)
#     ifeq ($(OSX_VERSION_MAJOR_GREATER_THAN_OR_EQUAL_TO_6),true)
#         LOCAL_LDLIBS += -Wl,-undefined,dynamic_lookup
#     endif
endif

ifeq ($(HOST_OS),windows)
    SDL2_CONFIG_LOADSO_WINDOWS := yes
    SDL2_CONFIG_THREAD_WINDOWS := yes
    SDL2_CONFIG_TIMER_WINDOWS := yes
    SDL2_CONFIG_VIDEO_WINDOWS := yes
    SDL2_CONFIG_RENDER_DIRECTX3D := yes
    SDL2_CONFIG_JOYSTICK_WINDOWS := yes
    SDL2_CONFIG_MAIN_WINDOWS := yes

    SDL2_CFLAGS += -D_GNU_SOURCE=1 -Dmain=SDL_main -DNO_STDIO_REDIRECT=1
    # NOTE: There is no EGL headers with our cross-toolchain.
    SDL2_CFLAGS += -DSDL_VIDEO_OPENGL_EGL=0 -DSDL_VIDEO_RENDER_OGL_ES2=0
    SDL2_LDLIBS += -luser32 -lgdi32 -lwinmm -lvfw32 -limm32 -lversion -lole32 -loleaut32
endif


# the main src/ sources
#
SRCS := SDL_assert.c \
        SDL.c \
        SDL_error.c \
        SDL_hints.c \
        SDL_log.c \

SRCS += atomic/SDL_atomic.c \
        atomic/SDL_spinlock.c \

SRCS += events/SDL_clipboardevents.c \
        events/SDL_dropevents.c \
        events/SDL_events.c \
        events/SDL_gesture.c \
        events/SDL_keyboard.c \
        events/SDL_mouse.c \
        events/SDL_quit.c \
        events/SDL_touch.c \
        events/SDL_windowevents.c \

SRCS += file/SDL_rwops.c

SRCS += stdlib/SDL_getenv.c \
        stdlib/SDL_iconv.c \
        stdlib/SDL_malloc.c \
        stdlib/SDL_qsort.c \
        stdlib/SDL_stdlib.c \
        stdlib/SDL_string.c \

SRCS += cpuinfo/SDL_cpuinfo.c

ifeq (windows,$(HOST_OS))
SRCS += core/windows/SDL_windows.c
endif

SDL2_SOURCES += $(SRCS:%=src/%)

# The src/libm sources

SRCS := e_atan2.c \
        e_log.c \
        e_pow.c \
        e_rem_pio2.c \
        e_sqrt.c \
        k_cos.c \
        k_rem_pio2.c \
        k_sin.c \
        s_atan.c \
        s_copysign.c \
        s_cos.c \
        s_fabs.c \
        s_floor.c \
        s_scalbn.c \
        s_sin.c \

SDL2_SOURCES += $(SRCS:%=src/libm/%)

# the src/dynapi/ sources

SRCS := SDL_dynapi.c

#SDL2_SOURCES += $(SRCS:%=src/dynapi/%)

# the LoadSO sources
#

SRCS :=

ifeq ($(SDL2_CONFIG_LOADSO_DLOPEN),yes)
  SRCS += dlopen/SDL_sysloadso.c
  SDL2_LDLIBS += -ldl
endif

ifeq ($(SDL2_CONFIG_LOADSO_WINDOWS),yes)
  SRCS += windows/SDL_sysloadso.c
endif

SDL2_SOURCES += $(SRCS:%=src/loadso/%)

# the Thread sources
#

SRCS := SDL_thread.c

ifeq ($(SDL2_CONFIG_THREAD_PTHREAD),yes)
  SRCS += pthread/SDL_syscond.c \
          pthread/SDL_sysmutex.c \
          pthread/SDL_syssem.c \
          pthread/SDL_systhread.c \
          pthread/SDL_systls.c \

endif

ifeq ($(SDL2_CONFIG_THREAD_WINDOWS),yes)
  SRCS += generic/SDL_syscond.c \
          windows/SDL_sysmutex.c \
          windows/SDL_syssem.c \
          windows/SDL_systhread.c \
          windows/SDL_systls.c \

endif

SDL2_SOURCES += $(SRCS:%=src/thread/%)

# the Timer sources
#

SRCS := SDL_timer.c

ifeq ($(SDL2_CONFIG_TIMER_UNIX),yes)
  SRCS += unix/SDL_systimer.c
endif

ifeq ($(SDL2_CONFIG_TIMER_WINDOWS),yes)
  SRCS += windows/SDL_systimer.c
endif

SDL2_SOURCES += $(SRCS:%=src/timer/%)

# the Video sources
#

SRCS := SDL_blit_0.c \
	SDL_blit_1.c \
	SDL_blit_A.c \
	SDL_blit_auto.c \
	SDL_blit.c \
	SDL_blit_copy.c \
	SDL_blit_N.c \
	SDL_blit_slow.c \
	SDL_bmp.c \
	SDL_clipboard.c \
	SDL_fillrect.c \
	SDL_pixels.c \
	SDL_rect.c \
	SDL_RLEaccel.c \
	SDL_shape.c \
	SDL_stretch.c \
	SDL_surface.c \
	SDL_video.c \

SRCS += dummy/SDL_nullevents.c \
        dummy/SDL_nullframebuffer.c \
        dummy/SDL_nullvideo.c

ifeq ($(SDL2_CONFIG_VIDEO_WINDOWS),yes)
  SRCS += windows/SDL_windowsclipboard.c \
          windows/SDL_windowsevents.c \
          windows/SDL_windowsframebuffer.c \
          windows/SDL_windowskeyboard.c \
          windows/SDL_windowsmessagebox.c \
          windows/SDL_windowsmodes.c \
          windows/SDL_windowsmouse.c \
          windows/SDL_windowsopengl.c \
          windows/SDL_windowsshape.c \
          windows/SDL_windowsvideo.c \
          windows/SDL_windowswindow.c \

endif

ifeq ($(SDL2_CONFIG_VIDEO_COCOA),yes)
  SRCS += cocoa/SDL_cocoaclipboard.m \
          cocoa/SDL_cocoaevents.m \
          cocoa/SDL_cocoakeyboard.m \
          cocoa/SDL_cocoamessagebox.m \
          cocoa/SDL_cocoamodes.m \
          cocoa/SDL_cocoamouse.m \
          cocoa/SDL_cocoamousetap.m \
          cocoa/SDL_cocoaopengl.m \
          cocoa/SDL_cocoashape.m \
          cocoa/SDL_cocoavideo.m \
          cocoa/SDL_cocoawindow.m \

endif

ifeq ($(SDL2_CONFIG_VIDEO_X11),yes)
  SRCS += x11/imKStoUCS.c \
          x11/SDL_x11clipboard.c \
          x11/SDL_x11dyn.c \
          x11/SDL_x11events.c \
          x11/SDL_x11framebuffer.c \
          x11/SDL_x11keyboard.c \
          x11/SDL_x11messagebox.c \
          x11/SDL_x11modes.c \
          x11/SDL_x11mouse.c \
          x11/SDL_x11opengl.c \
          x11/SDL_x11shape.c \
          x11/SDL_x11touch.c \
          x11/SDL_x11video.c \
          x11/SDL_x11window.c \
          x11/SDL_x11xinput2.c \

endif

SDL2_SOURCES += $(SRCS:%=src/video/%)

## The render sources
##

SRCS := SDL_d3dmath.c \
        SDL_render.c \
        SDL_yuv_mmx.c \
        SDL_yuv_sw.c \

SRCS += software/SDL_blendfillrect.c \
        software/SDL_blendline.c \
        software/SDL_blendpoint.c \
        software/SDL_drawline.c \
        software/SDL_drawpoint.c \
        software/SDL_render_sw.c \
        software/SDL_rotate.c \

SRCS += opengl/SDL_render_gl.c \
        opengl/SDL_shaders_gl.c \

ifeq ($(SDL2_CONFIG_RENDER_DIRECTX3D),yes)
SRCS += direct3d/SDL_render_d3d.c
endif

SDL2_SOURCES += $(SRCS:%=src/render/%)

## The joystick sources
##

SRCS := SDL_gamecontroller.c \
        SDL_joystick.c \

ifeq ($(SDL2_CONFIG_JOYSTICK_WINDOWS),yes)
SRCS += windows/SDL_dxjoystick.c \
        windows/SDL_mmjoystick.c \

endif

ifeq ($(SDL2_CONFIG_JOYSTICK_LINUX),yes)
SRCS += linux/SDL_sysjoystick.c
endif

ifeq ($(SDL2_CONFIG_JOYSTICK_DARWIN),yes)
SRCS += darwin/SDL_sysjoystick.c
endif

SDL2_SOURCES += $(SRCS:%=src/joystick/%)

## The haptic sources

SRCS := SDL_haptic.c \
        $(HOST_OS)/SDL_syshaptic.c

SDL2_SOURCES += $(SRCS:%=src/haptic/%)

## The audio sources

SRCS := SDL_audio.c

#SDL2_SOURCES += $(SRCS:%=src/audio/%)

## Main function
##

SRCS :=

ifeq ($(SDL2_CONFIG_MAIN_DUMMY),yes)
  SRCS += dummy/SDL_dummy_main.c
endif

ifeq ($(SDL2_CONFIG_MAIN_WINDOWS),yes)
  SRCS += windows/SDL_windows_main.c
endif

SDL2_SOURCES += $(SRCS:%=src/main/%)

$(call start-emulator-library,emulator_libSDL2)
LOCAL_SRC_FILES := $(SDL2_SOURCES)
LOCAL_CFLAGS += $(SDL2_CFLAGS)
LOCAL_C_INCLUDES := $(SDL2_INCLUDES)
$(call end-emulator-library)

$(call start-emulator64-library,emulator_lib64SDL2)
LOCAL_SRC_FILES := $(SDL2_SOURCES)
LOCAL_CFLAGS += $(SDL2_CFLAGS)
LOCAL_C_INCLUDES := $(SDL2_INCLUDES)
$(call end-emulator-library)

# Restore LOCAL_PATH
LOCAL_PATH := $(SDL2_OLD_LOCAL_PATH)
