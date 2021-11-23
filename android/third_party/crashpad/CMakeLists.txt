set(CRASHPAD_ROOT ${CMAKE_CURRENT_LIST_DIR})

if(WINDOWS_MSVC_X86_64)
  # Let's include the .dll path for our test runner
  string(REPLACE "/" "\\" CRASHPAD_TEST_DATA_ROOT "${CRASHPAD_ROOT}")
else()
  set(CRASHPAD_TEST_DATA_ROOT "${CRASHPAD_ROOT}")
endif()

# Make sure we do not have definitions that will break us.
remove_definitions(
  "-DNDEBUG"
  "-DNDEBUG=1"
  "-D_FILE_OFFSET_BITS=64"
  "-D_GNU_SOURCE=1"
  "-D_LARGEFILE_SOURCE"
  "-D_LIBCPP_VERSION=__GLIBCPP__"
  "-D__STDC_CONSTANT_MACROS"
  "-D__STDC_FORMAT_MACROS=1"
  "-D__STDC_LIMIT_MACROS=1"
  "-DANDROID_DEBUG")
  
add_subdirectory(third_party/mini_chromium)
add_subdirectory(client)
add_subdirectory(compat)
add_subdirectory(handler)
add_subdirectory(minidump)
add_subdirectory(snapshot)
add_subdirectory(test)
add_subdirectory(tools)
add_subdirectory(util)
