set(emulator-regex-win32_src
    compat/reallocarr.c
    compat/strlcpy.c
    src/regcomp.c
    src/regerror.c
    src/regexec.c
    src/regfree.c)
android_add_library(emulator-regex-win32)

android_target_include_directories(emulator-regex-win32 all PUBLIC include)
android_target_include_directories(emulator-regex-win32 all PRIVATE compat)
