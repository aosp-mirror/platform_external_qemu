	list(APPEND SOURCES
	constants.c
	# constants_unittest.cpp
	control.cpp
	# control_unittest.cpp
	globals.c
	)

add_library(network ${SOURCES})

