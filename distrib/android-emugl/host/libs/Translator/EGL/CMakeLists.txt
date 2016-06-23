if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(EGL_TRANSLATOR_PLATFORM_SRC EglOsApi_glx.cpp)
    set(EGL_TRANSLATOR_PLATFORM_LIB -lGL -lX11 -ldl -lpthread)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(EGL_TRANSLATOR_PLATFORM_SRC
        EglOsApi_darwin.cpp
        MacNative.m
        MacPixelFormatsAttribs.m
        )
    set(EGL_TRANSLATOR_PLATFORM_LIB -Wl,-framework,AppKit)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(EGL_TRANSLATOR_PLATFORM_SRC EglOsApi_wgl.cpp)
    set(EGL_TRANSLATOR_PLATFORM_LIB -lgdi32)
endif()

add_library(EGL_translator SHARED
    ${EGL_TRANSLATOR_PLATFORM_SRC}
    ThreadInfo.cpp
    EglImp.cpp
    EglConfig.cpp
    EglContext.cpp
    EglGlobalInfo.cpp
    EglValidate.cpp
    EglSurface.cpp
    EglWindowSurface.cpp
    EglPbufferSurface.cpp
    EglThreadInfo.cpp
    EglDisplay.cpp
    ClientAPIExts.cpp
    )

target_link_libraries(EGL_translator
    GLcommon
    android-emu-base
    ${EGL_TRANSLATOR_PLATFORM_LIB}
    )

target_include_directories(EGL_translator
    PRIVATE
    ../../../../shared # android-emugl/shared
    ../include         # android-emugl/host/libs/Translator/include
    ../../../../../..  # external/qemu
    )