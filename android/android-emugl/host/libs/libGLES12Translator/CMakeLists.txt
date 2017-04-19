set(SOURCES
  common/RefBase.cpp
  common/etc1.cpp
  common/log.cpp
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
  gles/translator_interface.cpp
  gles/uniform_value.cpp
  gles/vector.cpp
  )
include_directories(. ${GLESv1_dec_INCLUDE_DIRS})
add_library(GLES12TranslatorCommon SHARED ${SOURCES})
target_link_libraries(GLES12TranslatorCommon emugl_common ANDROID_BASE_LIBRARIES)
