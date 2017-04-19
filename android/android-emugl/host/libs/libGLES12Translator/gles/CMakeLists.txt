set(SOURCES
	api_entries.cpp
	buffer_data.cpp
	debug.cpp
	egl_image.cpp
	framebuffer_data.cpp
	gles1_shader_generator.cpp
	gles_context.cpp
	gles_validate.cpp
	matrix.cpp
	paletted_texture_util.cpp
	pass_through.cpp
	program_data.cpp
	program_variant.cpp
	renderbuffer_data.cpp
	shader_data.cpp
	shader_variant.cpp
	share_group.cpp
	state.cpp
	texture_codecs.cpp
	texture_data.cpp
	translator_interface.cpp
	uniform_value.cpp
	vector.cpp
	)

add_library(GLES12TranslatorGles ${SOURCES})
target_link_libraries(GLES12TranslatorGles emugl_common)
