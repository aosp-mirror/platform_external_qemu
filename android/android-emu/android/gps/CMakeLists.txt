list(APPEND SOURCES
        GpxParser.cpp
	# GpxParser_unittest.cpp
        KmlParser.cpp
	# KmlParser_unittest.cpp
	)

add_library(gps ${SOURCES})

