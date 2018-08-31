# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Linux)
set(triple x86_64-pc-win32)

set(TOOLCHAIN "${LOCAL_BUILD_OBJS_DIR}/build/toolchain")

# which compilers to use for C and C++
#find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)
set(CMAKE_RC_COMPILER ${TOOLCHAIN}/windres)
#find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
set(CMAKE_C_COMPILER ${TOOLCHAIN}/clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
#find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN}/clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})


# here is the target environment located, used to
# locate packages. We don't want to do any package resolution
# with mingw, so we explicitly disable it.
# set(CMAKE_FIND_ROOT_PATH  "UNUSED")

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_C_COMPILER_ID Clang)
set(CMAKE_CXX_COMPILER_ID Clang)

message("CMAKE_CROSSCOMPILING=${CMAKE_CROSSCOMPILING}")
message("CMAKE_C_COMPILER_ID=${CMAKE_C_COMPILER_ID}")
message("CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}")
