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

include_directories(${Protobuf_INCLUDE_DIR}
  ${CONFIG_HOST_INCLUDE_DIRS}
  )
add_library(metrics ${SOURCES})
target_link_libraries(metrics metrics_proto)



