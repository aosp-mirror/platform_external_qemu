add_library(android-emu-grpc-utils INTERFACE)
target_include_directories(android-emu-grpc-utils INTERFACE include)
target_link_libraries(android-emu-grpc-utils INTERFACE grpc++)
android_license(TARGET "android-emu-grpc-utils" LIBNAME None SPDX None LICENSE None LOCAL None)