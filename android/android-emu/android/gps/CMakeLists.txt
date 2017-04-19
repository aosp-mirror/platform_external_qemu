list(APPEND SOURCES
        GpxParser.cpp
	# GpxParser_unittest.cpp
        KmlParser.cpp
	# KmlParser_unittest.cpp
	)

include_directories(${XML_INCLUDE_DIRS})
add_library(gps ${SOURCES})

