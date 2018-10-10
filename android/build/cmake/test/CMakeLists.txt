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

#  We have a set of prebuilt libraries that we need. You can declare the ones you nee
# by using the prebuilt command:
prebuilt(UUID)

# This will result in resolving the UUID prebuilt library. If that succeeds (and it should)
# It will usually define the following set of variables:
# MODULENAME_LIBRARIES <-- The libs you need to link against.
# MODULENAME_INCLUDE_DIRS <-- The include directories where the headers are located
# MODULENAME_SHARED_DEPENDENCIES <-- Dependency you can add to your executable target. (Usally not set)
# MODULENAME_SHARED_PROPERTIESS <-- Dependency you can add to your executable target. (Usally not set)
# MODUELNAME_DEFINITIONS <-- Definitions that you would like to set on your target
`prebuilt(PROTOBUF)
prebuilt(QT5)


# Let's define a shared hello lib and all its settings.

# We are going to generate the protobuf headers and sources, and stuff them into a shared
# library.
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)
set(hello_base_src  ${PROTO_SRCS} ${PROTO_HDRS})

# and declare the library
android_add_shared_library(hello_base)

# The library needs to link against the protobuf libraries
target_link_libraries(hello_base PUBLIC ${PROTOBUF_LIBRARIES})

# We want to propagate the protobuf headers we are creating..
target_include_directories(hello_base PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${PROTOBUF_INCLUDE_DIRS})


# This merely showcases how you can modify the sources
# for the various android targets, in our case we always use the same.

set(hello_darwin-x86_64_src hello.cpp) # This source will only be in the darwin-x86_64 build
set(hello_linux-x86_64_src hello.cpp) `
set(hello_windows_src hello.cpp)  # This will end up in ALL windows builds
set(hello_windows-x86_64 hello.cpp)  # This will only end up in the mingw build

# TODO: not sure how to add __dlspec(dllexport) to the generated protobuf
# headers, so just include the src for now.
set(hello_windows_msvc-x86_64_src hello.cpp ${PROTO_SRCS})

# Add the executable
android_add_executable(hello)

# Compiler options that should not propagate to the next target, we need -fPIC
# on lin/darwin for QT, on windows this is not needed.
# Note that this command is basically the same as the CMake
# target_compile_options, with the difference that the 2nd argument
# specifies the android target.
android_target_compile_options(hello darwin-x86_64 PRIVATE -fPIC)

# On the linux target we configure the compiler to enable all warnings.
android_target_compile_options(hello linux-x86_64 PRIVATE  -fPIC -Wall) 


# The hello executable dependes on the following set of libraries
target_link_libraries(hello PRIVATE hello_base ${UUID_LIBRARIES} ${QT5_LIBRARIES})

# Includes that are private
target_include_directories(hello PRIVATE ${UUID_INCLUDE_DIR} ${QT5_INCLUDE_DIRS})

# The prebuilt dependencies that do magical things to our target (Such as setting rpaths, and copying over dependencies)
android_target_dependency(hello all "${QT5_SHARED_DEPENDENCIES}")
android_target_properties(hello all "${QT5_SHARED_PROPERTIES}")