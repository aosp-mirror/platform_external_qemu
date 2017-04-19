cmake_minimum_required (VERSION 2.8 FATAL_ERROR) 
project(EMUGEN CXX)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wno-variadic-macros -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCES
    ApiGen.cpp
    getopt.c
    EntryPoint.cpp
    main.cpp
    Parser.cpp
    strUtils.cpp
    TypeFactory.cpp)

include_directories(${EMUGEN_SOURCE_DIR}/../../../../android-emu)
add_executable(emugen ${SOURCES})
