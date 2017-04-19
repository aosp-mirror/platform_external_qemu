add_subdirectory(qt)
list(APPEND SOURCES
	charmap.c
	file.c
	image.c
	keyboard.c
	keycode-buffer.c
	keycode.c
	rect.c
	resource.c
	trackball.c
	ui.c
	window.c
	#keycode-buffer_unittest.cpp
	#keycode_unittest.cpp
	LibuiAgent.cpp
	#rect_unittest.cpp
	)
include_directories(${CONFIG_HOST_INCLUDE_DIRS})
add_library(skin ${SOURCES})



