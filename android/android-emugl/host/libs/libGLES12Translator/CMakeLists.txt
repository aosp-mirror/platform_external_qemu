set(GLES12Translator_src
    gles/api_entries.cpp
    gles/buffer_data.cpp
    gles/debug.cpp
    gles/egl_image.cpp
    gles/framebuffer_data.cpp
    gles/gles1_shader_generator.cpp
    gles/gles_context.cpp
    gles/gles_validate.cpp
    gles/matrix.cpp
    gles/paletted_texture_util.cpp
    gles/pass_through.cpp
    gles/program_data.cpp
    gles/program_variant.cpp
    gles/renderbuffer_data.cpp
    gles/shader_data.cpp
    gles/shader_variant.cpp
    gles/share_group.cpp
    gles/state.cpp
    gles/texture_codecs.cpp
    gles/texture_data.cpp
    gles/uniform_value.cpp
    gles/vector.cpp
    gles/translator_interface.cpp
    common/log.cpp
    common/etc1.cpp
    common/RefBase.cpp)
android_add_shared_library(GLES12Translator)
target_compile_options(GLES12Translator PRIVATE -fvisibility=hidden)
target_link_libraries(GLES12Translator PRIVATE emugl_base)
target_include_directories(GLES12Translator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GLES12Translator PUBLIC GLESv1_dec OpenglCodecCommon emugl_common)
android_install_shared(GLES12Translator)