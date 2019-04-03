set(ANDROID_EMUGL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(GOLDFISH_EMUGL_DIR ${ANDROID_QEMU2_TOP_DIR}/../../device/generic/goldfish-opengl/)

# We start of defining an interface library that defines all the compiler settings we wish to use for all libs. please
# keep this at a bare minimum!
android_add_interface(emugl_base)
target_compile_definitions(emugl_base INTERFACE "-DEMUGL_BUILD=1")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(emugl_base INTERFACE "-DEMUGL_DEBUG=1")
endif()

# Compile emugen for the current host.
android_compile_for_host(emugen ${CMAKE_CURRENT_LIST_DIR}/host/tools/emugen EMUGEN_EXE)

# This will generate the source files by executing the emugen builder.
function(generate_emugen SRC NAME)
  get_filename_component(DIR "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}" ABSOLUTE)

  list(APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_dec.cpp)
  list(APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_dec.h)
  list(APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_opcodes.h)
  list(APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_server_context.h)
  list(APPEND GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_server_context.cpp)

  add_custom_command(PRE_BUILD
                     OUTPUT ${GENERATED_SRC}
                     COMMAND ${EMUGEN_EXE} -D ${CMAKE_CURRENT_BINARY_DIR} -i ${DIR} ${NAME}
                     DEPENDS ${EMUGEN_EXE})

  set(${NAME}-SOURCES ${GENERATED_SRC} PARENT_SCOPE)
endfunction()

# Creates the library by generating the sources.
function(generate_emugen_lib LIBNAME LOCATION NAME SOURCES)
  generate_emugen(${LOCATION} ${NAME})
  set(${LIBNAME}_src ${${NAME}-SOURCES} ${SOURCES})
  android_add_library(${LIBNAME})
  target_include_directories(${LIBNAME}
                             PUBLIC ${LOCATION}
                                    host/include/OpenGLESDispatch
                                    host/include/
                                    host/libs/libGLSnapshot
                                    shared/OpenglCodecCommon
                                    ${CMAKE_CURRENT_BINARY_DIR})
  target_compile_options(${LIBNAME} PRIVATE -fvisibility=hidden)
  target_link_libraries(${LIBNAME} PUBLIC android-emu GLSnapshot)
  target_link_libraries(${LIBNAME} PRIVATE emugl_base)
endfunction()

# Generate the emugen libraries..
generate_emugen_lib(GLESv1_dec host/libs/GLESv1_dec gles1 host/libs/GLESv1_dec/GLESv1Decoder.cpp)
generate_emugen_lib(GLESv2_dec host/libs/GLESv2_dec gles2 host/libs/GLESv2_dec/GLESv2Decoder.cpp)
generate_emugen_lib(renderControl_dec host/libs/renderControl_dec renderControl "")

add_subdirectory(host/libs/Translator/EGL)
add_subdirectory(host/libs/Translator/GLES_CM)
add_subdirectory(host/libs/Translator/GLES_V2)
add_subdirectory(host/libs/Translator/GLcommon)
add_subdirectory(host/libs/libGLES12Translator)
add_subdirectory(host/libs/libGLSnapshot)
add_subdirectory(host/libs/libOpenGLESDispatch)
add_subdirectory(host/libs/libOpenglRender)
add_subdirectory(host/libs/libOpenglRender/vulkan/cereal)
add_subdirectory(shared/OpenglCodecCommon)
add_subdirectory(shared/emugl/common)

if(NOT WINDOWS)
  add_subdirectory(guest)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../../device/generic/goldfish-opengl goldfish-opengl)
  add_subdirectory(combined)
endif()
