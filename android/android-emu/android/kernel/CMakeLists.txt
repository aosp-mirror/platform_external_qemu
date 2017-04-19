list(APPEND SOURCES
        kernel_utils.cpp
	#kernel_utils_unittest.cpp
	)

add_library(kernel ${SOURCES})

