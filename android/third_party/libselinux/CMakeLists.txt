if (NOT WIN32) 
  list(APPEND SOURCES
    src/callbacks.c
    src/check_context.c
    src/freecon.c
    src/init.c
    src/label.c
    src/label_file.c
    src/label_android_property.c)
endif()

add_definitions(-DHOST)

if (APPLE)
  add_definitions(-DDARWIN)
endif()
add_library(SELINUX ${SOURCES})
