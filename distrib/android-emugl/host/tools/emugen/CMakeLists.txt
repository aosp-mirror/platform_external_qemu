cmake_minimum_required(VERSION 3.3)
project (emugen)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_compile_options(
    -std=c++11
    -lstdc++
    )

add_executable(emugen
    ApiGen.cpp
    EntryPoint.cpp
    main.cpp
    Parser.cpp
    strUtils.cpp
    TypeFactory.cpp
    )

