cmake_minimum_required(VERSION 3.5)
project(TEST)

# This here is to make sure we can find the dependencies.
# You don't need this if you are using any of our toolchains.
# (i.e. you invoked this using cmake -DCMAKE_TOOLCHAIN_FILE=${QEMU2_TOP_DIR}/android/build/cmake/toolchain-yyy-x86_64.cmake)
set(TOOLS_DIRECTORY ${TEST_SOURCE_DIR}/../..)
list(APPEND CMAKE_MODULE_PATH "${TOOLS_DIRECTORY}/cmake")
include(android)

# Must be c++14 or later for msvc build or clang will throw
set(CMAKE_CXX_STANDARD 14)

# We have a set of prebuilt libraries that we need. You can declare the ones you nee
# by using the prebuilt command:

# The prebuilt command mimics the find_package (FindCURL, FindLibXml2, etc) commands from cmake. They will produce similar results
# but will bind to the local archives. If you build without our toolgenerator it will
# fall back to cmake's resolution mechanism. This is dangerous as you might up with a different version of
# the library than we use.

# For example:
prebuilt(PROTOBUF)
# Mimics https://cmake.org/cmake/help/v3.13/module/FindProtobuf.html
# And will define the protobuf::libprotobuf target, which you can take a dependency on. CMake
# will setup your includes and linker flags.


# The following below will bring in the Qt5 dependencies. We only define a few of the standard
# QT5 libraries (Qt5::Widget, Qt5Core, Qt5::Svg, Qt5Gui).
prebuilt(QT5)

# Note, that if corresponding FindXXXX does not really exist we default to the target name: PKG::PKG
# So:
prebuilt(UUID)
# Will define a UUID::UUID target.

# Let's define a shared hello lib and all its settings.

# We are going to generate the protobuf headers and sources, and stuff them into a shared
# library.
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)
set(hello_base_src  ${PROTO_SRCS} ${PROTO_HDRS})

# and declare the library
android_add_shared_library(hello_base)

# The library needs to link against the protobuf libraries
# Note that this brings in the protobuf dependency, including all the
# necessary header files.

# Ideally you make all the dependencies private. In a private dependency
# none of the include directories, compiler settings or compiler flags will propagate.
# cmake will still use your library to link against if someone takes a dependency on you.
target_link_libraries(hello_base PRIVATE protobuf::libprotobuf)

# This merely showcases how you can modify the sources
# for the various android targets, in our case we always use the same.

set(hello_darwin-x86_64_src hello.cpp) # This source will only be in the darwin-x86_64 build
set(hello_linux-x86_64_src hello.cpp)
set(hello_windows_src hello.cpp)  # This will end up in ALL windows builds

# TODO: not sure how to add __dlspec(dllexport) to the generated protobuf
# headers, so just include the src for now.
set(hello_windows_msvc-x86_64_src hello.cpp ${PROTO_SRCS})

# Add the executable
android_add_executable(hello)

# Compiler options that should not propagate to the next target, we need -fPIC
# on linux/darwin for QT, on windows this is not needed.
# Note that this command is basically the same as the CMake
# target_compile_options, with the difference that the 2nd argument
# specifies the android target.
android_target_compile_options(hello darwin-x86_64 PRIVATE -fPIC)

# On the linux target we configure the compiler to enable all warnings.
android_target_compile_options(hello linux-x86_64 PRIVATE  -fPIC -Wall)


# The hello executable dependes on the following set of targets.
# The include dependencies, and compiler definitions needed will be brought in
target_link_libraries(hello PRIVATE hello_base UUID::UUID Qt5::Core)

# The prebuilt dependencies that do magical things to our target (Such as setting rpaths, and copying over dependencies)
android_target_dependency(hello all  QT5_SHARED_DEPENDENCIES)
android_target_properties(hello all "${QT5_SHARED_PROPERTIES}")
