
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
    target_include_directories(emulator-regex-win32 PRIVATE msvc)
else()
    target_include_directories(emulator-regex-win32 PRIVATE compat)
endif()
android_target_link_libraries(emulator-regex-win32 windows_msvc PUBLIC
        msvc-posix-compat)