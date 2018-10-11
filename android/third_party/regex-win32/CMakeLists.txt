set(emulator-regex-win32_src
    compat/reallocarr.c
    compat/strlcpy.c
    src/regcomp.c
    src/regerror.c
    src/regexec.c
    src/regfree.c)

set(emulator-regex-win32_includes_public include)
set(emulator-regex-win32_includes_private compat)
add_android_library(emulator-regex-win32)
