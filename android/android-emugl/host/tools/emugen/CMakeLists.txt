cmake_minimum_required(VERSION 3.5)
project(emugen)
enable_testing ()

add_subdirectory(../../../../../../googletest build/googletest)
include_directories(../../../../android-emu)

add_executable(emugen
    ApiGen.cpp
    EntryPoint.cpp
    main.cpp
    Parser.cpp
    strUtils.cpp
    TypeFactory.cpp)

add_executable(emugen_unittests
    Parser.cpp
    Parser_unittest.cpp)

IF(UNIX AND NOT APPLE)
    set_target_properties(emugen PROPERTIES LINK_FLAGS "-Wl,-rpath=$ORIGIN/lib64:$ORIGIN/lib:$ORIGIN/intermediates64")
    set_target_properties(emugen_unittests PROPERTIES LINK_FLAGS "-Wl,-rpath=$ORIGIN/lib64:$ORIGIN/lib:$ORIGIN/intermediates64")
ENDIF(UNIX AND NOT APPLE)


target_link_libraries(emugen_unittests gmock_main)
add_test (NAME emugen_unittests COMMAND emugen_unittests)
