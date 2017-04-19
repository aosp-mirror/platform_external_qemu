list(APPEND SOURCES
  EmuglBackendList.cpp
  #EmuglBackendList_unittest.cpp
  EmuglBackendScanner.cpp
  #EmuglBackendScanner_unittest.cpp
  GLProcessPipe.cpp
  GpuFrameBridge.cpp
  #GpuFrameBridge_unittest.cpp
  OpenglEsPipe.cpp
  emugl_config.cpp
  #emugl_config_unittest.cpp
  gpuinfo.cpp
  #gpuinfo_unittest.cpp
  logger.cpp
  )

add_library(opengl ${SOURCES})


