cmake_minimum_required(VERSION 3.5)
project(emugen)
enable_testing ()

add_executable(emugen
    ApiGen.cpp
    EntryPoint.cpp
    main.cpp
    Parser.cpp
    strUtils.cpp
    TypeFactory.cpp)

target_prebuilt_dependency(emugen "${RUNTIME_OS_DEPENDENCIES}" "${RUNTIME_OS_PROPERTIES}")
target_link_libraries(emugen PRIVATE android-emu)

add_executable(emugen_unittests
    Parser.cpp
    Parser_unittest.cpp)

target_link_libraries(emugen_unittests gmock_main)
add_test (NAME emugen_unittests COMMAND emugen_unittests)
