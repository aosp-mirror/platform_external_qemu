
set(emulator-regex-win32_src
    compat/reallocarr.c
    compat/strlcpy.c
    src/regcomp.c
    src/regerror.c
    src/regexec.c
    src/regfree.c)
android_add_library(emulator-regex-win32)

target_include_directories(emulator-regex-win32 PUBLIC include)
if(MSVC)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_include_directories(emulator-regex-win32 PRIVATE compat)
  else()
    target_include_directories(emulator-regex-win32 PRIVATE msvc)
  endif()
  target_link_libraries(emulator-regex-win32 PUBLIC msvc-posix-compat)
else()
    target_include_directories(emulator-regex-win32 PRIVATE compat)
    android_target_link_libraries(emulator-regex-win32 windows_msvc PUBLIC msvc-posix-compat)
endif()
