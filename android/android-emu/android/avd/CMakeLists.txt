list(APPEND SOURCES
  hw-config.c
  info.c
  scanner.c
  util.c
  #  util_unittest.cpp
  util_wrapper.cpp
  # util_wrapper_unittest.cpp 
  )


  add_library(avd ${SOURCES})


