#----------------------------------------------------------------
# Generated CMake target import file for configuration "None".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "jsoncpp_lib" for configuration "None"
set_property(TARGET jsoncpp_lib APPEND PROPERTY IMPORTED_CONFIGURATIONS NONE)
set_target_properties(jsoncpp_lib PROPERTIES
  IMPORTED_LOCATION_NONE "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libjsoncpp.so.1.7.4"
  IMPORTED_SONAME_NONE "libjsoncpp.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS jsoncpp_lib )
list(APPEND _IMPORT_CHECK_FILES_FOR_jsoncpp_lib "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libjsoncpp.so.1.7.4" )

# Import target "jsoncpp_lib_static" for configuration "None"
set_property(TARGET jsoncpp_lib_static APPEND PROPERTY IMPORTED_CONFIGURATIONS NONE)
set_target_properties(jsoncpp_lib_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NONE "CXX"
  IMPORTED_LOCATION_NONE "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libjsoncpp.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS jsoncpp_lib_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_jsoncpp_lib_static "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libjsoncpp.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
