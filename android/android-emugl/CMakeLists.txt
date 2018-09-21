cmake_minimum_required (VERSION 3.5)
project (emugl)

if(${ANDROID_TARGET_TAG} MATCHES "windows.*")
  include (ExternalProject)

  # If we are cross compiling we will need to build it for our actual OS
  set (EMUGEN_EXE ${CMAKE_BINARY_DIR}/emugen/src/emugen-build/emugen)
  externalproject_add (
    emugen
    PREFIX
    emugen
    DOWNLOAD_COMMAND
    ""
    SOURCE_DIR
    "${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/host/tools/emugen"
    CMAKE_ARGS
    "-DCMAKE_TOOLCHAIN_FILE=${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/toolchain-linux-x86_64.cmake"
    BUILD_BYPRODUCTS
    emugen
    INSTALL_COMMAND
    ""
  )
else ()
  # Include the emugen executable..
  add_subdirectory ("host/tools/emugen")
  #set (EMUGEN_EXE ${CMAKE_CURRENT_BINARY_DIR}/host/tools/emugen/emugen)
  set (EMUGEN_EXE emugen)
endif()

function(generate_emugen SRC NAME)
  get_filename_component (DIR "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}" ABSOLUTE)

  list (APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_dec.cpp)
  list (APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_dec.h)
  list (APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_opcodes.h)
  list (
    APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_server_context.h
  )
  list (
    APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_server_context.cpp
  )

  add_custom_command (
    PRE_BUILD
    OUTPUT ${GENERATED_SRC}
    COMMAND
      ${EMUGEN_EXE} -D ${CMAKE_CURRENT_BINARY_DIR} -i ${DIR} ${NAME}
    DEPENDS emugen
  )

  set (${NAME}-SOURCES ${GENERATED_SRC} PARENT_SCOPE)
endfunction()

function(generate_emugen_lib LIBNAME LOCATION NAME SOURCES)
  generate_emugen (${LOCATION} ${NAME})
  add_library (${LIBNAME} ${${NAME}-SOURCES} ${SOURCES})
  target_include_directories (
    ${LIBNAME}
    PUBLIC
      ${LOCATION}
      host/include/OpenGLESDispatch
      host/include/
      host/libs/libGLSnapshot
      shared/OpenglCodecCommon
      ${CMAKE_CURRENT_BINARY_DIR}
  )
  target_link_libraries (${LIBNAME} PUBLIC android-emu)
endfunction()

# Generate the emugen libraries..
generate_emugen_lib (
  libGLESv1_dec
  host/libs/GLESv1_dec
  gles1
  host/libs/GLESv1_dec/GLESv1Decoder.cpp
)
generate_emugen_lib (
  libGLESv2_dec
  host/libs/GLESv2_dec
  gles2
  host/libs/GLESv2_dec/GLESv2Decoder.cpp
)
generate_emugen_lib (
  librenderControl_dec
  host/libs/renderControl_dec
  renderControl
  ""
)

add_library (
  libemugl_common
  shared/emugl/common/crash_reporter.cpp
  shared/emugl/common/dma_device.cpp
  shared/emugl/common/feature_control.cpp
  shared/emugl/common/logging.cpp
  shared/emugl/common/misc.cpp
  shared/emugl/common/shared_library.cpp
  shared/emugl/common/stringparsing.cpp
  shared/emugl/common/sync_device.cpp
)
target_include_directories (libemugl_common PUBLIC shared)
target_link_libraries (libemugl_common PUBLIC android-emu)

if(APPLE OR ${ANDROID_TARGET_TAG} STREQUAL "darwin-x86_64")
  target_link_libraries (libemugl_common PUBLIC -ldl -lpthread)
  # Nothing..
elseif (WIN32 OR ${ANDROID_TARGET_TAG} MATCHES "windows.*")

elseif (UNIX OR ${ANDROID_TARGET_TAG} STREQUAL "linux-x86_64")
  target_link_libraries (libemugl_common PUBLIC -ldl -lpthread)
endif()

add_executable (
  emugl_common_host_unittests shared/emugl/common/shared_library_unittest.cpp
  shared/emugl/common/stringparsing_unittest.cpp
)
target_link_libraries (
  emugl_common_host_unittests
  PRIVATE libemugl_common gtest gmock_main
)

add_library (
  libemugl_test_shared_library SHARED
  shared/emugl/common/testing/test_shared_library.cpp
)
target_compile_options (
  libemugl_test_shared_library
  PRIVATE "-fvisibility=default"
)

add_library (
  libOpenglCodecCommon
  shared/OpenglCodecCommon/glUtils.cpp
  shared/OpenglCodecCommon/ChecksumCalculator.cpp
  shared/OpenglCodecCommon/ChecksumCalculatorThreadInfo.cpp
)
target_include_directories (
  libOpenglCodecCommon
  PUBLIC host/include/OpenglRender
)
target_link_libraries (libOpenglCodecCommon PUBLIC libemugl_common)

add_library (
  libGLcommon
  host/libs/Translator/GLcommon/etc.cpp
  host/libs/Translator/GLcommon/FramebufferData.cpp
  host/libs/Translator/GLcommon/GLBackgroundLoader.cpp
  host/libs/Translator/GLcommon/GLDispatch.cpp
  host/libs/Translator/GLcommon/GLutils.cpp
  host/libs/Translator/GLcommon/GLEScontext.cpp
  host/libs/Translator/GLcommon/GLESvalidate.cpp
  host/libs/Translator/GLcommon/GLESpointer.cpp
  host/libs/Translator/GLcommon/GLESbuffer.cpp
  host/libs/Translator/GLcommon/NamedObject.cpp
  host/libs/Translator/GLcommon/ObjectData.cpp
  host/libs/Translator/GLcommon/ObjectNameSpace.cpp
  host/libs/Translator/GLcommon/PaletteTexture.cpp
  host/libs/Translator/GLcommon/RangeManip.cpp
  host/libs/Translator/GLcommon/SaveableTexture.cpp
  host/libs/Translator/GLcommon/ScopedGLState.cpp
  host/libs/Translator/GLcommon/ShareGroup.cpp
  host/libs/Translator/GLcommon/TextureData.cpp
  host/libs/Translator/GLcommon/TextureUtils.cpp
)

target_include_directories (libGLcommon PUBLIC host/libs/Translator/include)
target_link_libraries (
  libGLcommon
  PUBLIC
    android-emu
    emulator-astc-codec
    libemugl_common
)

if(APPLE OR ${ANDROID_TARGET_TAG} STREQUAL "darwin-x86_64")
  # Nothing..
elseif (WIN32 OR ${ANDROID_TARGET_TAG} MATCHES "windows.*")
  target_link_libraries (libGLcommon PUBLIC -lgdi32 -Wl,--add-stdcall-alias)
elseif (UNIX OR ${ANDROID_TARGET_TAG} STREQUAL "linux-x86_64")
  target_link_libraries (libGLcommon PUBLIC -ldl -Wl,-Bsymbolic)
endif()

add_library (
  libGLES_CM_translator SHARED
  host/libs/Translator/GLES_CM/CoreProfileEngine.cpp
  host/libs/Translator/GLES_CM/GLEScmImp.cpp
  host/libs/Translator/GLES_CM/GLEScmUtils.cpp
  host/libs/Translator/GLES_CM/GLEScmContext.cpp
  host/libs/Translator/GLES_CM/GLEScmValidate.cpp
)
target_compile_definitions(libGLES_CM_translator PRIVATE
                           -D__STDC_CONSTANT_MACROS
                           -D_LIBCPP_VERSION=__GLIBCPP__
                           -DNDEBUG=1
                           -D__STDC_LIMIT_MACROS=1
                           -D__STDC_FORMAT_MACROS=1
                           -D_FILE_OFFSET_BITS=64
                           -D_LARGEFILE_SOURCE
                           -D_GNU_SOURCE=1
                           "-D__packed=__attribute\(\(packed\)\)"
                           -D__STDC_LIMIT_MACROS=1
                           -DEMUGL_BUILD=1)
target_link_libraries (
  libGLES_CM_translator
  PUBLIC libGLcommon android-emu emulator-astc-codec
)
# Of course our naming changes depending on our bitness.
set_target_properties(libGLES_CM_translator PROPERTIES
                                 OUTPUT_NAME ${TARGET_BITS}GLES_CM_translator
                                 LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib${TARGET_BITS})

prebuilt (ANGLE)
add_library (
    libGLES_V2_translator SHARED
  host/libs/Translator/GLES_V2/GLESv2Imp.cpp
  host/libs/Translator/GLES_V2/GLESv2Context.cpp
  host/libs/Translator/GLES_V2/GLESv2Validate.cpp
  host/libs/Translator/GLES_V2/SamplerData.cpp
  host/libs/Translator/GLES_V2/ShaderParser.cpp
  host/libs/Translator/GLES_V2/ShaderValidator.cpp
  host/libs/Translator/GLES_V2/ProgramData.cpp
  host/libs/Translator/GLES_V2/ANGLEShaderParser.cpp
)

target_compile_definitions(libGLES_V2_translator PRIVATE
                           -D__STDC_CONSTANT_MACROS
                           -D_LIBCPP_VERSION=__GLIBCPP__
                           -DNDEBUG=1
                           -D__STDC_LIMIT_MACROS=1
                           -D__STDC_FORMAT_MACROS=1
                           -D_FILE_OFFSET_BITS=64
                           -D_LARGEFILE_SOURCE
                           -D_GNU_SOURCE=1
                           "-D__packed=__attribute\(\(packed\)\)"
                           -D__STDC_LIMIT_MACROS=1
                           -DEMUGL_BUILD=1)
target_include_directories (
  libGLES_V2_translator
  PRIVATE ${ANGLE_INCLUDE_DIR} host/libs/Translator/include host/include
)
target_link_libraries (
  libGLES_V2_translator
  PUBLIC
    libGLcommon
    libOpenglCodecCommon
    android-emu
    emulator-astc-codec
    ${ANGLE_LIBRARIES}
)

# Of course our naming changes depending on our bitness.
set_target_properties(libGLES_V2_translator PROPERTIES
                                 OUTPUT_NAME ${TARGET_BITS}GLES_V2_translator
                                 LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib${TARGET_BITS})

# EGL
set (
  EGL_SOURCES
  host/libs/Translator/EGL/ThreadInfo.cpp
  host/libs/Translator/EGL/EglImp.cpp
  host/libs/Translator/EGL/EglConfig.cpp
  host/libs/Translator/EGL/EglContext.cpp
  host/libs/Translator/EGL/EglGlobalInfo.cpp
  host/libs/Translator/EGL/EglValidate.cpp
  host/libs/Translator/EGL/EglSurface.cpp
  host/libs/Translator/EGL/EglWindowSurface.cpp
  host/libs/Translator/EGL/EglPbufferSurface.cpp
  host/libs/Translator/EGL/EglThreadInfo.cpp
  host/libs/Translator/EGL/EglDisplay.cpp
  host/libs/Translator/EGL/ClientAPIExts.cpp
)

if(APPLE OR ${ANDROID_TARGET_TAG} STREQUAL "darwin-x86_64")
  set (
    EGL_SOURCES
    ${EGL_SOURCES}
    host/libs/Translator/EGL/EglOsApi_darwin.cpp
    host/libs/Translator/EGL/EglOsApi_egl.cpp
    host/libs/Translator/EGL/MacNative.m
    host/libs/Translator/EGL/MacPixelFormatsAttribs.m
  )
  add_library (libEGL_translator SHARED ${EGL_SOURCES})
  target_link_libraries (libEGL_translator PUBLIC "-framework AppKit")
elseif (WIN32 OR ${ANDROID_TARGET_TAG} MATCHES "windows.*")
  set (
    EGL_SOURCES
    ${EGL_SOURCES}
    host/libs/Translator/EGL/glOsApi_wgl.cpp
    host/libs/Translator/EGL/EglOsApi_egl.cpp
    host/libs/Translator/EGL/CoreProfileConfigs_windows.cpp
  )
  add_library (libEGL_translator SHARED ${EGL_SOURCES})
  target_link_libraries (libEGL_translator PUBLIC -lgdi32)

elseif (UNIX OR ${ANDROID_TARGET_TAG} STREQUAL "linux-x86_64")
  set (
    EGL_SOURCE
    ${EGL_SOURCES}
    host/libs/Translator/EGL/EglOsApi_glx.cpp
    host/libs/Translator/EGL/EglOsApi_egl.cpp
    host/libs/Translator/EGL/CoreProfileConfigs_linux.cpp
  )
  add_library (libEGL_translator SHARED ${EGL_SOURCES})
  target_link_libraries (
    libEGL_translator
    PUBLIC
      -lGL
      -lX11
      -ldl
      -lpthread
  )
  message(STATUS "Building for linux!")
endif()

target_link_libraries (
  libEGL_translator
  PUBLIC libGLcommon android-emu emulator-astc-codec
)

target_compile_definitions(libEGL_translator PRIVATE
                           -D__STDC_CONSTANT_MACROS
                           -D_LIBCPP_VERSION=__GLIBCPP__
                           -DNDEBUG=1
                           -D__STDC_LIMIT_MACROS=1
                           -D__STDC_FORMAT_MACROS=1
                           -D_FILE_OFFSET_BITS=64
                           -D_LARGEFILE_SOURCE
                           -D_GNU_SOURCE=1
                           "-D__packed=__attribute\(\(packed\)\)"
                           -D__STDC_LIMIT_MACROS=1
                           -DEMUGL_BUILD=1)

# Of course our naming changes depending on our bitness.
set_target_properties(libEGL_translator PROPERTIES
                                 OUTPUT_NAME ${TARGET_BITS}EGL_translator
                                 LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib${TARGET_BITS})


# OpenGLESDispatch
add_library (
  libOpenGLESDispatch
  host/libs/libOpenGLESDispatch/EGLDispatch.cpp
  host/libs/libOpenGLESDispatch/GLESv2Dispatch.cpp
  host/libs/libOpenGLESDispatch/GLESv1Dispatch.cpp
  host/libs/libOpenGLESDispatch/OpenGLDispatchLoader.cpp
)
target_link_libraries (libOpenGLESDispatch PUBLIC libGLESv1_dec libGLESv2_dec)

# Vulkan

add_library (
  libOpenglRender_vulkan_cereal
  host/libs/libOpenglRender/vulkan/cereal/common/goldfish_vk_marshaling.cpp
  host/libs/libOpenglRender/vulkan/cereal/guest/goldfish_vk_frontend.cpp
  host/libs/libOpenglRender/vulkan/cereal/common/goldfish_vk_testing.cpp
  host/libs/libOpenglRender/vulkan/cereal/common/goldfish_vk_deepcopy.cpp
  host/libs/libOpenglRender/vulkan/cereal/common/goldfish_vk_handlemap.cpp
  host/libs/libOpenglRender/vulkan/cereal/common/goldfish_vk_dispatch.cpp
)

target_link_libraries (
  libOpenglRender_vulkan_cereal
  PUBLIC android-emu android-emu-base
)
target_include_directories (
  libOpenglRender_vulkan_cereal
  PUBLIC
    host/include/vulkan host/libs/libOpenglRender/vulkan
    host/libs/libOpenglRender/vulkan/cereal
)

add_library (
  libOpenglRender_vulkan host/libs/libOpenglRender/vulkan/VulkanDispatch.cpp
  host/libs/libOpenglRender/vulkan/VulkanStream.cpp
)

target_link_libraries (
  libOpenglRender_vulkan
  PUBLIC android-emu android-emu-base libOpenglRender_vulkan_cereal
)
target_include_directories (
  libOpenglRender_vulkan
  PUBLIC
    host/include/vulkan host/libs/libOpenglRender/vulkan
    host/libs/libOpenglRender/vulkan/cereal
)

add_executable (
  OpenglRender_vulkan_unittests
  host/libs/libOpenglRender/vulkan/VulkanStream_unittest.cpp
)
target_link_libraries (
  OpenglRender_vulkan_unittests
  PUBLIC libOpenglRender_vulkan gmock_main
)

add_library (libGLSnapshot host/libs/libGLSnapshot/GLSnapshot.cpp)
target_link_libraries (
  libGLSnapshot
  PUBLIC libOpenGLESDispatch libemugl_common android-emu-base
)

target_link_libraries(libGLESv2_dec PUBLIC libGLSnapshot)
add_subdirectory (host/libs/libOpenglRender/)
