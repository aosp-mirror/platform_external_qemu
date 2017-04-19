list(APPEND sources
  # testing/main-test-crasher.cpp
  ui/ConfirmDialog.cpp
  CrashReporter_common.cpp
  CrashReporter_windows.cpp
  CrashService_common.cpp
  # CrashService_unittest.cpp
  CrashSystem.cpp
  CrashSystem.h
  # CrashSystem_unittest.cpp
  main-crash-service.cpp)


if(APPLE)
  list(APPEND SOURCES
    CrashReporter_darwin.cpp
    )
elseif(UNIX)
  list(APPEND SOURCES
    CrashReporter_linux.cpp
    )
elseif(WIN32)
  list(APPEND SOURCES
    CrashService_windows.cpp
    )
endif()


add_library(crashreport ${SOURCES} ${GENERATED_SOURCES})

