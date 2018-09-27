# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

set(TOOLCHAIN "${LOCAL_BUILD_OBJS_DIR}/build/toolchain")
set(COMPILER_PREFIX "${TOOLCHAIN}/x86_64-w64-mingw32")

# which compilers to use for C and C++
#find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)
set(CMAKE_RC_COMPILER ${COMPILER_PREFIX}-windres)
#find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
set(CMAKE_C_COMPILER ${COMPILER_PREFIX}-gcc)
#find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
set(CMAKE_CXX_COMPILER ${COMPILER_PREFIX}-g++)


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

