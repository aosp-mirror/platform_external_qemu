add_library(emugl_common
	crash_reporter.cpp
	dma_device.cpp
	feature_control.cpp
	logging.cpp
	misc.cpp
	shared_library.cpp
        # 	shared_library_unittest.cpp
	sockets.cpp
	stringparsing.cpp
        # stringparsing_unittest.cpp
	sync_device.cpp
	)
target_link_libraries(emugl_common dl pthread)
