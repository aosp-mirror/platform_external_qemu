#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Snappy::snappy" for configuration "RelWithDebInfo"
set_property(TARGET Snappy::snappy APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Snappy::snappy PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "/usr/lib/x86_64-linux-gnu/libsnappy.so.1.1.7"
  IMPORTED_SONAME_RELWITHDEBINFO "libsnappy.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS Snappy::snappy )
list(APPEND _IMPORT_CHECK_FILES_FOR_Snappy::snappy "/usr/lib/x86_64-linux-gnu/libsnappy.so.1.1.7" )

# Import target "Snappy::snappy-static" for configuration "RelWithDebInfo"
set_property(TARGET Snappy::snappy-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Snappy::snappy-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "/usr/lib/x86_64-linux-gnu/libsnappy.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS Snappy::snappy-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_Snappy::snappy-static "/usr/lib/x86_64-linux-gnu/libsnappy.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
