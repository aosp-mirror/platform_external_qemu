list(APPEND SOURCES
	AdbLivenessChecker.cpp
	AsyncMetricsReporter.cpp
	CrashMetricsReporting.cpp
	FileMetricsWriter.cpp
	MemoryUsageReporter.cpp
	metrics.cpp
	MetricsPaths.cpp
	MetricsReporter.cpp
	MetricsWriter.cpp
	NullMetricsReporter.cpp
	NullMetricsWriter.cpp
	Percentiles.cpp
	PeriodicReporter.cpp
	StudioConfig.cpp
	# 	StudioConfig_unittest.cpp
	SyncMetricsReporter.cpp
	TextMetricsWriter.cpp
	)


add_subdirectory(proto)

# The generated proto headers end up in a directory below us, so we need
# to make sure we pick them up
include_directories($<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>/android-emu)

# We need the build specific config host, we have multiple of these
# and they are not yet properly automatically generated.
# In this case we want the one that is checked into our branch.
include_directories(${CONFIG_HOST_INCLUDE_DIRS})
add_library(metrics ${SOURCES})



