# This cmake file is to configure cross-compilation for Windows on Linux
set(WIN32 1)
set(MSVC 1)

set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")
set(CMAKE_SHARED_LIBRARY_PREFIX "")          # lib
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")          # .so
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")          # .exe
set(CMAKE_LINK_LIBRARY_SUFFIX ".lib")
set(CMAKE_DL_LIBS "")

set(CMAKE_FIND_LIBRARY_PREFIXES "")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
