list(APPEND SOURCES
	qt_path.cpp
	#qt_path_unittest.cpp
	qt_setup.cpp
	#qt_setup_unittest.cpp
	)

# We need the checked in config-host
add_library(qt ${SOURCES})

