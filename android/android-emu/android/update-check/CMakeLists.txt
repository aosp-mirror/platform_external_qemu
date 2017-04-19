list(APPEND SOURCES
	UpdateChecker.cpp
	UpdateChecker.h
	#UpdateChecker_unittest.cpp
	VersionExtractor.cpp
	VersionExtractor.h
	#VersionExtractor_unittest.cpp
	)

# We need the checked in config-host
include_directories(${CONFIG_HOST_INCLUDE_DIRS}
	${XML_INCLUDE_DIRS}
	)
add_library(update-check ${SOURCES})

