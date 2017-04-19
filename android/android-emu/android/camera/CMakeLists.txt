list(APPEND SOURCES
  camera-common.cpp
  camera-format-converters.c
  camera-list.cpp
  camera-service.c
  )



if(APPLE)
  list(APPEND SOURCES camera-capture-mac.m)
elseif(UNIX)
  list(APPEND SOURCES camera-capture-linux.c)
elseif(WIN32)
  list(APPEND SOURCES camera-capture-windows.cpp)
endif()

add_library(camera ${SOURCES})

