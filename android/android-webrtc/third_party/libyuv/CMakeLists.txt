# CMakeLists for libyuv
# Originally created for "roxlu build system" to compile libyuv on windows
# Run with -DTEST=ON to build unit tests
project(YUV C CXX) # "C" is required even for C++ projects
set(ly_base_dir ${PROJECT_SOURCE_DIR})
set(ly_src_dir ${ly_base_dir}/source)
set(ly_inc_dir ${ly_base_dir}/include)
set(ly_tst_dir ${ly_base_dir}/unit_test)
set(ly_lib_name webrtc-yuv)
set(ly_lib_static ${ly_lib_name})
set(ly_lib_shared ${ly_lib_name}_shared)

file(GLOB_RECURSE ly_source_files ${ly_src_dir}/*.cc)
list(SORT ly_source_files)

file(GLOB_RECURSE ly_unittest_sources ${ly_tst_dir}/*.cc)
list(SORT ly_unittest_sources)

add_library(${ly_lib_static} STATIC ${ly_source_files})
target_include_directories(${ly_lib_static} PUBLIC ${ly_inc_dir})

if (NOT WINDOWS_MSVC_X86_64)
    add_executable(libyuv_unittest ${ly_unittest_sources})
    target_link_libraries(libyuv_unittest ${ly_lib_name} gtest_main
        Threads::Threads)
endif()
