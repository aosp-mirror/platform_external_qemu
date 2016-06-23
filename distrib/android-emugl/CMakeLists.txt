add_compile_options(-DEMUGL_BUILD=1)

# Needed to ensure SIZE_MAX is properly defined when including <stdint.h>
add_compile_options(-D__STDC_LIMIT_MACROS=1) # duplicate?

include_directories(host/include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-DEMUGL_DEBUG=1)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_compile_options(-fvisibility=internal)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_compile_options(-fvisibility=hidden)
endif()

# make the GenerateDecoder function available to all subdirectories
include(host/tools/emugen/GenerateDecoder.cmake)

add_subdirectory(shared/emugl/common)
add_subdirectory(shared/OpenglCodecCommon)
add_subdirectory(host/libs/GLESv1_dec)
add_subdirectory(host/libs/GLESv2_dec)
add_subdirectory(host/libs/libOpenGLESDispatch)
add_subdirectory(host/libs/renderControl_dec)
add_subdirectory(host/libs/libOpenglRender)
add_subdirectory(host/libs/Translator/EGL)
add_subdirectory(host/libs/Translator/GLcommon)
add_subdirectory(host/libs/Translator/GLES_CM)
add_subdirectory(host/libs/Translator/GLES_V2)

include(ExternalProject)
ExternalProject_Add(emugen
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/host/tools/emugen
    BINARY_DIR ${CMAKE_BINARY_DIR}/host/emugen
    INSTALL_COMMAND ""
    CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_HOST_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_HOST_CXX_COMPILER}
    )

